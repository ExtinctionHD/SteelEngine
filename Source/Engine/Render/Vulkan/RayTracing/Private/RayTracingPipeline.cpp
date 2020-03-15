#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"

#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"

namespace SRayTracingPipeline
{
    std::vector<vk::RayTracingShaderGroupCreateInfoNV> BuildShaderGroupsCreateInfo(
            const std::vector<RayTracingShaderGroup> &shaderGroups)
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

    std::optional<vk::DeviceSize> GetShaderGroupOffset(const std::vector<RayTracingShaderGroup> &shaderGroups,
            const std::function<bool(const RayTracingShaderGroup &)> groupPred, uint32_t handleSize)
    {
        const auto it = std::find_if(shaderGroups.begin(), shaderGroups.end(), groupPred);

        if (it != shaderGroups.end())
        {
            return std::distance(shaderGroups.begin(), it) * handleSize;
        }

        return std::nullopt;
    }

    ShaderBindingTable GenerateSBT(const Device &device, BufferManager &bufferManager, vk::Pipeline pipeline,
            const std::vector<ShaderModule> &shaderModules, const std::vector<RayTracingShaderGroup> &shaderGroups)
    {
        const uint32_t handleSize = device.GetRayTracingProperties().shaderGroupHandleSize;
        const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());

        Bytes shaderGroupsData(handleSize * groupCount);

        const vk::Result result = device.Get().getRayTracingShaderGroupHandlesNV(pipeline,
                0, groupCount, shaderGroupsData.size(), shaderGroupsData.data());

        Assert(result == vk::Result::eSuccess);

        const BufferDescription description{
            shaderGroupsData.size(),
            vk::BufferUsageFlagBits::eRayTracingNV | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const BufferHandle buffer = bufferManager.CreateBuffer(description,
                BufferCreateFlags::kNone, GetByteView(shaderGroupsData));

        const auto raygenPred = [&shaderModules](const RayTracingShaderGroup &shaderGroup)
            {
                return shaderGroup.type == vk::RayTracingShaderGroupTypeNV::eGeneral
                        && shaderModules[shaderGroup.generalShader].stage == vk::ShaderStageFlagBits::eRaygenNV;
            };
        const std::optional<vk::DeviceSize> raygenOffset = GetShaderGroupOffset(shaderGroups, raygenPred, handleSize);
        Assert(raygenOffset.has_value());

        const auto missPred = [&shaderModules](const RayTracingShaderGroup &shaderGroup)
            {
                return shaderGroup.type == vk::RayTracingShaderGroupTypeNV::eGeneral
                        && shaderModules[shaderGroup.generalShader].stage == vk::ShaderStageFlagBits::eMissNV;
            };
        const std::optional<vk::DeviceSize> missOffset = GetShaderGroupOffset(shaderGroups, missPred, handleSize);
        Assert(missOffset.has_value());

        const auto hitPred = [](const RayTracingShaderGroup &shaderGroup)
            {
                return shaderGroup.type == vk::RayTracingShaderGroupTypeNV::eTrianglesHitGroup;
            };
        const std::optional<vk::DeviceSize> hitOffset = GetShaderGroupOffset(shaderGroups, hitPred, handleSize);
        Assert(hitOffset.has_value());

        return ShaderBindingTable{ buffer, raygenOffset.value(), missOffset.value(), hitOffset.value(), handleSize };
    }
}

std::unique_ptr<RayTracingPipeline> RayTracingPipeline::Create(std::shared_ptr<Device> device,
        BufferManager &bufferManager, const RayTracingPipelineDescription &description)
{
    const auto shaderStagesCreateInfo = ShaderHelpers::BuildShaderStagesCreateInfo(description.shaderModules);
    const auto shaderGroupsCreateInfo = SRayTracingPipeline::BuildShaderGroupsCreateInfo(description.shaderGroups);

    const vk::PipelineLayout layout = VulkanHelpers::CreatePipelineLayout(device->Get(),
            description.layouts, description.pushConstantRanges);

    const vk::RayTracingPipelineCreateInfoNV createInfo({},
            static_cast<uint32_t>(shaderStagesCreateInfo.size()), shaderStagesCreateInfo.data(),
            static_cast<uint32_t>(shaderGroupsCreateInfo.size()), shaderGroupsCreateInfo.data(),
            1, layout);

    const auto [result, pipeline] = device->Get().createRayTracingPipelineNV({}, createInfo);
    Assert(result == vk::Result::eSuccess);

    const ShaderBindingTable shaderBindingTable = SRayTracingPipeline::GenerateSBT(GetRef(device),
            bufferManager, pipeline, description.shaderModules, description.shaderGroups);

    return std::unique_ptr<RayTracingPipeline>(new RayTracingPipeline(device, pipeline, layout, shaderBindingTable));
}

RayTracingPipeline::RayTracingPipeline(std::shared_ptr<Device> device_, vk::Pipeline pipeline_,
        vk::PipelineLayout layout_, const ShaderBindingTable &shaderBindingTable_)
    : device(device_)
    , pipeline(pipeline_)
    , layout(layout_)
    , shaderBindingTable(shaderBindingTable_)
{}

RayTracingPipeline::~RayTracingPipeline()
{
    device->Get().destroyPipelineLayout(layout);
    device->Get().destroyPipeline(pipeline);
}
