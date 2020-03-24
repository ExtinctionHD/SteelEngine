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

    ShaderModule CreateShaderModule(vk::ShaderStageFlagBits stage, const Filepath &filepath,
            const std::vector<std::pair<std::string, uint32_t>> &defines);

private:
    struct ShaderCacheEntry
    {
        Filepath filepath;
        std::vector<std::pair<std::string, uint32_t>> defines;
        ShaderModule shaderModule;
    };

    Filepath baseDirectory;

    std::list<ShaderCacheEntry> shaderCache;
    std::unordered_map<Filepath, std::string> codeCache;
};
