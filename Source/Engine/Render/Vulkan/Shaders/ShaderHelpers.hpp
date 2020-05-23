#pragma once

struct ShaderModule
{
    vk::ShaderStageFlagBits stage;
    vk::ShaderModule module;
};

namespace ShaderHelpers
{
    std::vector<vk::PipelineShaderStageCreateInfo> BuildShaderStagesCreateInfo(
            const std::vector<ShaderModule> &shaderModules);
}
