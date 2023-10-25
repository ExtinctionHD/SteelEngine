#pragma once

#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"

using ShaderDefines = std::map<std::string, uint32_t>;

class ShaderManager
{
public:
    ShaderManager(const Filepath& baseDirectory_);
    ~ShaderManager();

    ShaderModule CreateShaderModule(const Filepath& filepath, vk::ShaderStageFlagBits stage, const ShaderDefines& defines = ShaderDefines{}) const;

    ShaderModule CreateComputeShaderModule(const Filepath& filepath, const glm::uvec3& workGroupSize, const ShaderDefines& defines = ShaderDefines{}) const;

    void DestroyShaderModule(const ShaderModule& shaderModule) const;

private:
    Filepath baseDirectory;
};
