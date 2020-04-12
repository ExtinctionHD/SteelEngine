#pragma once

#include "Engine/Filesystem.hpp"

struct ShaderModule
{
    vk::ShaderStageFlagBits stage;
    vk::ShaderModule module;
};

class ShaderCache
{
public:
    ShaderCache(const Filepath &baseDirectory_);
    ~ShaderCache();

    ShaderModule CreateShaderModule(vk::ShaderStageFlagBits stage, const Filepath &filepath);

private:
    Filepath baseDirectory;

    std::unordered_map<Filepath, ShaderModule> shaderCache;
};
