#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"

#include "Utils/Filesystem.hpp"

class ShaderCache
{
public:
    static std::unique_ptr<ShaderCache> Create(std::shared_ptr<Device> device, const Filepath &baseDirectory);

    ShaderCache(std::shared_ptr<Device> aDevice, const Filepath &aBaseDirectory);
    ~ShaderCache();

    ShaderModule CreateShader(vk::ShaderStageFlagBits stage, const Filepath &filepath,
            const std::vector<std::pair<std::string, uint32_t>> &defines);

private:
    struct ShaderCacheEntry
    {
        Filepath filepath;
        std::vector<std::pair<std::string, uint32_t>> defines;
        ShaderModule shaderModule;
    };

    std::shared_ptr<Device> device;

    Filepath baseDirectory;

    std::list<ShaderCacheEntry> shaderCache;
    std::unordered_map<Filepath, std::string> codeCache;
};
