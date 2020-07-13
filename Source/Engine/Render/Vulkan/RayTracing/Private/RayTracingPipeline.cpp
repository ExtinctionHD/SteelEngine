#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"

#include "Utils/Assert.hpp"

namespace SRayTracingPipeline
{
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> BuildShaderGroupsCreateInfo(
            const std::vector<RayTracingPipeline::ShaderGroup> &shaderGroups)
    {
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> createInfo;
        createInfo.reserve(shaderGroups.size());

        for (const auto &shaderGroup : shaderGroups)
        {
            createInfo.emplace_back(shaderGroup.type, shaderGroup.generalShader,
                    shaderGroup.closestHitShader, shaderGroup.anyHitShader, VK_SHADER_UNUSED_KHR);
        }

        return createInfo;
    }

    std::optional<vk::DeviceSize> GetShaderGroupOffset(const std::vector<RayTracingPipeline::ShaderGroup> &shaderGroups,
            const std::function<bool(const RayTracingPipeline::ShaderGroup &)> groupPred, uint32_t baseAlignment)
    {
        const auto it = std::find_if(shaderGroups.begin(), shaderGroups.end(), groupPred);

        if (it != shaderGroups.end())
        {
            return std::distance(shaderGroups.begin(), it) * baseAlignment;
        }

        return std::nullopt;
    }

    vk::Buffer CreateShaderGroupsBuffer(const Bytes &shaderGroupsData)
    {
        const BufferDescription description{
            shaderGroupsData.size(),
            vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(
                description, BufferCreateFlagBits::eStagingBuffer);

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                VulkanContext::bufferManager->UpdateBuffer(commandBuffer, buffer, GetByteView(shaderGroupsData));

                const PipelineBarrier barrier{
                    SyncScope::kTransferWrite,
                    SyncScope::kRayTracingShaderRead
                };

                BufferHelpers::InsertPipelineBarrier(commandBuffer, buffer, barrier);
            });

        return buffer;
    }

    Bytes RealignShaderGroupsData(const Bytes &source, uint32_t handleSize, uint32_t baseAlignment)
    {
        Assert(source.size() % handleSize == 0);
        Assert(baseAlignment % handleSize == 0);

        const uint32_t multiplier = baseAlignment / handleSize;

        Bytes result(source.size() * multiplier);

        for (size_t i = 0; i < source.size() / handleSize; ++i)
        {
            std::memcpy(result.data() + i * baseAlignment, source.data() + i * handleSize, handleSize);
        }

        return result;
    }

    ShaderBindingTable GenerateSBT(vk::Pipeline pipeline,
            const std::vector<ShaderModule> &shaderModules,
            const std::vector<RayTracingPipeline::ShaderGroup> &shaderGroups)
    {
        const uint32_t handleSize = VulkanContext::device->GetRayTracingProperties().shaderGroupHandleSize;
        const uint32_t baseAlignment = VulkanContext::device->GetRayTracingProperties().shaderGroupBaseAlignment;
        const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());

        Bytes shaderGroupsData(handleSize * groupCount);

        const vk::Result result = VulkanContext::device->Get().getRayTracingShaderGroupHandlesKHR<uint8_t>(
                pipeline, 0, groupCount, shaderGroupsData);

        Assert(result == vk::Result::eSuccess);

        shaderGroupsData = RealignShaderGroupsData(shaderGroupsData, handleSize, baseAlignment);

        const vk::Buffer buffer = CreateShaderGroupsBuffer(shaderGroupsData);

        const auto raygenPred = [&shaderModules](const RayTracingPipeline::ShaderGroup &shaderGroup)
            {
                return shaderGroup.type == vk::RayTracingShaderGroupTypeKHR::eGeneral
                        && shaderModules[shaderGroup.generalShader].stage == vk::ShaderStageFlagBits::eRaygenKHR;
            };
        const std::optional<vk::DeviceSize> raygenOffset = GetShaderGroupOffset(shaderGroups, raygenPred, baseAlignment);
        Assert(raygenOffset.has_value());

        const auto missPred = [&shaderModules](const RayTracingPipeline::ShaderGroup &shaderGroup)
            {
                return shaderGroup.type == vk::RayTracingShaderGroupTypeKHR::eGeneral
                        && shaderModules[shaderGroup.generalShader].stage == vk::ShaderStageFlagBits::eMissKHR;
            };
        const std::optional<vk::DeviceSize> missOffset = GetShaderGroupOffset(shaderGroups, missPred, baseAlignment);
        Assert(missOffset.has_value());

        const auto hitPred = [](const RayTracingPipeline::ShaderGroup &shaderGroup)
            {
                return shaderGroup.type == vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
            };
        const std::optional<vk::DeviceSize> hitOffset = GetShaderGroupOffset(shaderGroups, hitPred, baseAlignment);
        Assert(hitOffset.has_value());

        return ShaderBindingTable{ buffer, raygenOffset.value(), missOffset.value(), hitOffset.value(), baseAlignment };
    }
}

std::unique_ptr<RayTracingPipeline> RayTracingPipeline::Create(const Description &description)
{
    const auto shaderStagesCreateInfo = ShaderHelpers::BuildShaderStagesCreateInfo(description.shaderModules);
    const auto shaderGroupsCreateInfo = SRayTracingPipeline::BuildShaderGroupsCreateInfo(description.shaderGroups);

    const vk::Device device = VulkanContext::device->Get();
    const vk::PipelineLayout layout = VulkanHelpers::CreatePipelineLayout(device,
            description.layouts, description.pushConstantRanges);

    const vk::RayTracingPipelineCreateInfoKHR createInfo({},
            static_cast<uint32_t>(shaderStagesCreateInfo.size()), shaderStagesCreateInfo.data(),
            static_cast<uint32_t>(shaderGroupsCreateInfo.size()), shaderGroupsCreateInfo.data(),
            8, vk::PipelineLibraryCreateInfoKHR(), nullptr, layout);

    const auto [result, pipeline] = device.createRayTracingPipelineKHR(vk::PipelineCache(), createInfo);
    Assert(result == vk::Result::eSuccess);

    const ShaderBindingTable shaderBindingTable = SRayTracingPipeline::GenerateSBT(pipeline,
            description.shaderModules, description.shaderGroups);

    return std::unique_ptr<RayTracingPipeline>(new RayTracingPipeline(pipeline, layout, shaderBindingTable));
}

RayTracingPipeline::RayTracingPipeline(vk::Pipeline pipeline_, vk::PipelineLayout layout_,
        const ShaderBindingTable &shaderBindingTable_)
    : pipeline(pipeline_)
    , layout(layout_)
    , shaderBindingTable(shaderBindingTable_)
{}

RayTracingPipeline::~RayTracingPipeline()
{
    VulkanContext::device->Get().destroyPipelineLayout(layout);
    VulkanContext::device->Get().destroyPipeline(pipeline);
}
