#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"
#include "Engine/Filesystem/Filepath.hpp"

using ShaderDefines = std::map<std::string, uint32_t>;

class ShaderManager
{
public:
    ShaderManager();
    ~ShaderManager();

    ShaderModule CreateShaderModule(const Filepath& filepath, vk::ShaderStageFlagBits stage,
            const ShaderDefines& defines = ShaderDefines{}) const;

    ShaderModule CreateComputeShaderModule(const Filepath& filepath,
            const ShaderDefines& defines = ShaderDefines{},
            const glm::uvec3& workGroupSize = PipelineHelpers::kDefaultWorkGroupSize) const;

    void DestroyShaderModule(const ShaderModule& shaderModule) const;

private:
    Filepath baseDirectory;
};
