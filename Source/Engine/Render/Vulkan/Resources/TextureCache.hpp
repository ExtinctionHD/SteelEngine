#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Render/Vulkan/Resources/Texture.hpp"
#include "Engine/Filesystem.hpp"

class TextureCache
{
public:
    TextureCache(std::shared_ptr<Device> device_, std::shared_ptr<ImageManager> imageManager_);
    ~TextureCache();

    Texture GetTexture(const Filepath &filepath, const SamplerDescription &samplerDescription);

    vk::Sampler GetSampler(const SamplerDescription &description);

private:
    std::shared_ptr<Device> device;
    std::shared_ptr<ImageManager> imageManager;

    std::unordered_map<Filepath, ImageHandle> textures;
    std::unordered_map<SamplerDescription, vk::Sampler> samplers;
};
