#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"

#include "Engine/Render/Vulkan/Shaders/ShaderCache.hpp"

std::vector<vk::PipelineShaderStageCreateInfo> ShaderHelpers::BuildShaderStagesCreateInfo(
        const std::vector<ShaderModule>& shaderModules)
{
    std::vector<vk::PipelineShaderStageCreateInfo> createInfo;
    createInfo.reserve(shaderModules.size());

    for (const auto& shaderModule : shaderModules)
    {
        createInfo.emplace_back(vk::PipelineShaderStageCreateFlags(),
            shaderModule.stage, shaderModule.module, "main", nullptr);
    }

    return createInfo;
}