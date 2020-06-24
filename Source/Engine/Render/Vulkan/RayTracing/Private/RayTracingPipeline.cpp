#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"

#include "Utils/Assert.hpp"

namespace SRayTracingPipeline
{
    std::vector<vk::RayTracingShaderGroupCreateInfoNV> BuildShaderGroupsCreateInfo(
            const std::vector<RayTracingPipeline::ShaderGroup> &shaderGroups)
    {
        std::vector<vk::RayTracingShaderGroupCreateInfoNV> createInfo;
        createInfo.reserve(shaderGroups.size());

        for (const auto &shaderGroup : shaderGroups)
        {
            createInfo.emplace_back(shaderGroup.type, shaderGroup.generalShader,
                    shaderGroup.closestHitShader, shaderGroup.anyHitShader, VK_SHADER_UNUSED_NV);
        }

        return createInfo;
    }

    std::optional<vk::DeviceSize> GetShaderGroupOffset(const std::vector<RayTracingPipeline::ShaderGroup> &shaderGroups,
            const std::function<bool(const RayTracingPipeline::ShaderGroup &)> groupPred, uint32_t handleSize)
    {
        const auto it = std::find_if(shaderGroups.begin(), shaderGroups.end(), groupPred);

        if (it != shaderGroups.end())
        {
            return std::distance(shaderGroups.begin(), it) * handleSize;
        }

        return std::nullopt;
    }

    vk::Buffer CreateShaderGroupsBuffer(const Bytes &shaderGroupsData)
    {
        const BufferDescription bufferDescription{
            shaderGroupsData.size(),
            vk::BufferUsageFlagBits::eRayTracingNV | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(
                bufferDescription, BufferCreateFlagBits::eStagingBuffer);

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                VulkanContext::bufferManager->UpdateBuffer(commandBuffer, buffer, ByteView(shaderGroupsData));

                const PipelineBarrier barrier{
                    SyncScope::kTransferWrite,
                    SyncScope::kRayTracingShaderRead
                };

                BufferHelpers::InsertPipelineBarrier(commandBuffer, buffer, barrier);
            });

        return buffer;
    }

    ShaderBindingTable GenerateSBT(vk::Pipeline pipeline,
            const std::vector<ShaderModule> &shaderModules,
            const std::vector<RayTracingPipeline::ShaderGroup> &shaderGroups)
    {
        const uint32_t handleSize = VulkanContext::device->GetRayTracingProperties().shaderGroupHandleSize;
        const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());

        Bytes shaderGroupsData(handleSize * groupCount);

        const vk::Result result = VulkanContext::device->Get().getRayTracingShaderGroupHandlesNV<uint8_t>(
                pipeline, 0, groupCount, shaderGroupsData);

        Assert(result == vk::Result::eSuccess);

        const vk::Buffer buffer = CreateShaderGroupsBuffer(shaderGroupsData);

        const auto raygenPred = [&shaderModules](const RayTracingPipeline::ShaderGroup &shaderGroup)
            {
                return shaderGroup.type == vk::RayTracingShaderGroupTypeNV::eGeneral
                        && shaderModules[shaderGroup.generalShader].stage == vk::ShaderStageFlagBits::eRaygenNV;
            };
        const std::optional<vk::DeviceSize> raygenOffset = GetShaderGroupOffset(shaderGroups, raygenPred, handleSize);
        Assert(raygenOffset.has_value());

        const auto missPred = [&shaderModules](const RayTracingPipeline::ShaderGroup &shaderGroup)
            {
                return shaderGroup.type == vk::RayTracingShaderGroupTypeNV::eGeneral
                        && shaderModules[shaderGroup.generalShader].stage == vk::ShaderStageFlagBits::eMissNV;
            };
        const std::optional<vk::DeviceSize> missOffset = GetShaderGroupOffset(shaderGroups, missPred, handleSize);
        Assert(missOffset.has_value());

        const auto hitPred = [](const RayTracingPipeline::ShaderGroup &shaderGroup)
            {
                return shaderGroup.type == vk::RayTracingShaderGroupTypeNV::eTrianglesHitGroup;
            };
        const std::optional<vk::DeviceSize> hitOffset = GetShaderGroupOffset(shaderGroups, hitPred, handleSize);
        Assert(hitOffset.has_value());

        return ShaderBindingTable{ buffer, raygenOffset.value(), missOffset.value(), hitOffset.value(), handleSize };
    }
}

std::unique_ptr<RayTracingPipeline> RayTracingPipeline::Create(const Description &description)
{
    const auto shaderStagesCreateInfo = ShaderHelpers::BuildShaderStagesCreateInfo(description.shaderModules);
    const auto shaderGroupsCreateInfo = SRayTracingPipeline::BuildShaderGroupsCreateInfo(description.shaderGroups);

    const vk::Device device = VulkanContext::device->Get();
    const vk::PipelineLayout layout = VulkanHelpers::CreatePipelineLayout(device,
            description.layouts, description.pushConstantRanges);

    const vk::RayTracingPipelineCreateInfoNV createInfo({},
            static_cast<uint32_t>(shaderStagesCreateInfo.size()), shaderStagesCreateInfo.data(),
            static_cast<uint32_t>(shaderGroupsCreateInfo.size()), shaderGroupsCreateInfo.data(),
            8, layout);

    const auto [result, pipeline] = device.createRayTracingPipelineNV(vk::PipelineCache(), createInfo);
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
