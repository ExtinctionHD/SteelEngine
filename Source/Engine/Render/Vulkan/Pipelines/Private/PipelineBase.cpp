#include "Engine/Render/Vulkan/Pipelines/PipelineBase.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

PipelineBase::~PipelineBase()
{
    for (const auto& descriptorSetLayout : descriptorSetLayouts)
    {
        VulkanContext::descriptorPool->DestroyDescriptorSetLayout(descriptorSetLayout);
    }
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

std::unique_ptr<DescriptorProvider> PipelineBase::CreateDescriptorProvider() const
{
    return std::make_unique<DescriptorProvider>(reflection.descriptors, descriptorSetLayouts);
}

PipelineBase::PipelineBase(vk::Pipeline pipeline_, vk::PipelineLayout layout_,
        const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts_,
        const ShaderReflection& reflection_)
    : pipeline(pipeline_)
    , layout(layout_)
    , descriptorSetLayouts(descriptorSetLayouts_)
    , reflection(reflection_)
{}
