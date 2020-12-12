#include "Engine/Render/Vulkan/Resources/Utils/DirectLightRetriever.hpp"

#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace Details
{
    const Filepath kShaderPath("~/Shaders/Compute/DirectLight.comp");

    std::unique_ptr<ComputePipeline> CreateComputePipeline(const std::vector<vk::DescriptorSetLayout> layouts)
    {
        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
            vk::ShaderStageFlagBits::eCompute, kShaderPath);

        const vk::PushConstantRange pushConstantRange(vk::ShaderStageFlagBits::eCompute,
            0, sizeof(vk::Extent2D) + sizeof(uint32_t));

        const ComputePipeline::Description pipelineDescription{
            shaderModule, layouts, { pushConstantRange }
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(pipelineDescription);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }
}

DirectLightRetriever::DirectLightRetriever() {}

DirectLightRetriever::~DirectLightRetriever() {}

glm::vec4 DirectLightRetriever::Retrieve(const Texture&) const
{
    return {};
}
