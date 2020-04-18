#pragma once

#include "Engine/Render/Vulkan/Resources/Texture.hpp"
#include "Engine/Filesystem.hpp"

class TextureCache
{
public:
    TextureCache() = default;
    ~TextureCache();

    Texture GetTexture(const Filepath &filepath, const SamplerDescription &samplerDescription);

    Texture GetEnvironmentMap(const Filepath& filepath, const SamplerDescription& samplerDescription);

    vk::Sampler GetSampler(const SamplerDescription &description);

private:
    struct TextureEntry
    {
        vk::Image image;
        vk::ImageView view;
    };

    std::unordered_map<Filepath, TextureEntry> textures;
    std::unordered_map<SamplerDescription, vk::Sampler> samplers;
};
