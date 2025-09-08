#include "Engine/Render/Vulkan/Pipelines/PipelineBase.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

PipelineBase::~PipelineBase()
{
    VulkanContext::device->Get().destroyPipelineLayout(layout);
    VulkanContext::device->Get().destroyPipeline(pipeline);
}

void PipelineBase::Bind(vk::CommandBuffer commandBuffer) const
{
    commandBuffer.bindPipeline(GetBindPoint(), pipeline);
}

void PipelineBase::BindDescriptorSet(vk::CommandBuffer commandBuffer,
        uint32_t firstSet, vk::DescriptorSet descriptorSet) const
{
    commandBuffer.bindDescriptorSets(GetBindPoint(), layout, firstSet, { descriptorSet }, {});
}

void PipelineBase::BindDescriptorSets(vk::CommandBuffer commandBuffer,
        const std::vector<vk::DescriptorSet>& descriptorSets) const
{
    commandBuffer.bindDescriptorSets(GetBindPoint(), layout, 0, descriptorSets, {});
}

void PipelineBase::BindDescriptorSlice(vk::CommandBuffer commandBuffer, uint32_t sliceIndex) const
{
    commandBuffer.bindDescriptorSets(GetBindPoint(), layout, 0, GetDescriptorSlice(sliceIndex), {});
}

PipelineBase::PipelineBase(vk::Pipeline pipeline_, vk::PipelineLayout layout_,
        const ShaderReflection& reflection_)
    : DescriptorProvider(reflection_.descriptors)
    , pipeline(pipeline_)
    , layout(layout_)
    , pushConstants(reflection_.pushConstants)
{}
