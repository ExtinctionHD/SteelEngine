#pragma once

struct ShaderModule;

namespace ShaderHelpers
{
    std::vector<vk::PipelineShaderStageCreateInfo> BuildShaderStagesCreateInfo(
            const std::vector<ShaderModule>& shaderModules);
}