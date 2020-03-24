#pragma once

#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Render/Vulkan/Resources/Texture.hpp"
#include "Engine/Filesystem.hpp"

class TextureCache
{
public:
    TextureCache() = default;
    ~TextureCache();

    Texture GetTexture(const Filepath &filepath, const SamplerDescription &samplerDescription);

    vk::Sampler GetSampler(const SamplerDescription &description);

private:
    std::unordered_map<Filepath, ImageHandle> textures;
    std::unordered_map<SamplerDescription, vk::Sampler> samplers;
};
