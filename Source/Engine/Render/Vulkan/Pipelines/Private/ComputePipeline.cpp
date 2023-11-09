#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"

#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

std::unique_ptr<ComputePipeline> ComputePipeline::Create(const ShaderModule& shaderModule)
{
    const std::vector<ShaderModule> shaderModules{ shaderModule };

    const vk::PipelineShaderStageCreateInfo shaderStageCreateInfo =
            ShaderHelpers::CreateShaderStagesCreateInfo(shaderModules).front();

    const std::vector<vk::DescriptorSetLayout> descriptorSetLayouts =
            ShaderHelpers::GetDescriptorSetLayouts(shaderModule.reflection.descriptors);

    const std::vector<vk::PushConstantRange> pushConstantRanges =
            ShaderHelpers::GetPushConstantRanges(shaderModule.reflection.pushConstants);

    const vk::PipelineLayout layout =
            VulkanHelpers::CreatePipelineLayout(VulkanContext::device->Get(), descriptorSetLayouts, pushConstantRanges);

    const vk::ComputePipelineCreateInfo createInfo({}, shaderStageCreateInfo, layout);

    const auto [result, pipeline] = VulkanContext::device->Get().createComputePipeline(nullptr, createInfo);
    Assert(result == vk::Result::eSuccess);

    return std::unique_ptr<ComputePipeline>(
            new ComputePipeline(pipeline, layout, descriptorSetLayouts, shaderModule.reflection));
}

ComputePipeline::ComputePipeline(vk::Pipeline pipeline_, vk::PipelineLayout layout_,
        const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts_, const ShaderReflection& reflection_)
    : PipelineBase(pipeline_, layout_, descriptorSetLayouts_, reflection_)
{}
