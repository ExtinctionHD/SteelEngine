#include "Engine/Render/Vulkan/ComputePipeline.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"

std::unique_ptr<ComputePipeline> ComputePipeline::Create(const Description& description)
{
    const vk::PipelineShaderStageCreateInfo shaderStageCreateInfo
            = ShaderHelpers::CreateShaderStagesCreateInfo({ description.shaderModule }).front();

    const vk::PipelineLayout layout = VulkanHelpers::CreatePipelineLayout(VulkanContext::device->Get(),
            description.layouts, description.pushConstantRanges);

    const vk::ComputePipelineCreateInfo createInfo({}, shaderStageCreateInfo, layout);

    const auto [result, pipeline] = VulkanContext::device->Get().createComputePipeline(nullptr, createInfo);
    Assert(result == vk::Result::eSuccess);

    return std::unique_ptr<ComputePipeline>(new ComputePipeline(pipeline, layout));
}

ComputePipeline::ComputePipeline(vk::Pipeline pipeline_, vk::PipelineLayout layout_)
    : pipeline(pipeline_)
    , layout(layout_)
{}

ComputePipeline::~ComputePipeline()
{
    VulkanContext::device->Get().destroyPipelineLayout(layout);
    VulkanContext::device->Get().destroyPipeline(pipeline);
}
