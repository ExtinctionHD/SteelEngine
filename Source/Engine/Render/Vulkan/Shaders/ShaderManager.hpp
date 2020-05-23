#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"
#include "Engine/Filesystem/Filepath.hpp"

class ShaderManager
{
public:
    ShaderManager(const Filepath &baseDirectory_);

    ShaderModule CreateShaderModule(vk::ShaderStageFlagBits stage, const Filepath &filepath) const;

    void DestroyShaderModule(const ShaderModule &shaderModule) const;

private:
    Filepath baseDirectory;
};
