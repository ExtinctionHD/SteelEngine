#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"

#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"

#include "Utils/Assert.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"

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
}

std::unique_ptr<RayTracingPipeline> RayTracingPipeline::Create(std::shared_ptr<Device> device,
        const RayTracingPipelineDescription &description)
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

    return std::unique_ptr<RayTracingPipeline>(new RayTracingPipeline(device, pipeline, layout));
}

RayTracingPipeline::RayTracingPipeline(std::shared_ptr<Device> aDevice, vk::Pipeline aPipeline,
        vk::PipelineLayout aLayout)
    : device(aDevice)
    , pipeline(aPipeline)
    , layout(aLayout)
{}

RayTracingPipeline::~RayTracingPipeline()
{
    device->Get().destroyPipelineLayout(layout);
    device->Get().destroyPipeline(pipeline);
}
