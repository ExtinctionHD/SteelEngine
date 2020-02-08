#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Filesystem.hpp"

struct Texture
{
    ImageHandle image;
    vk::Sampler sampler;
};

struct SamplerDescription
{
    vk::Filter magFilter = vk::Filter::eLinear;
    vk::Filter minFilter = vk::Filter::eLinear;
    vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear;
    vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat;
    std::optional<float> maxAnisotropy = 16.0f;
    uint32_t minLod = 0;
    uint32_t maxLod = 0;
};

class TextureCache
{
public:
    TextureCache(std::shared_ptr<Device> aDevice, std::shared_ptr<ImageManager> aImageManager);
    ~TextureCache();

    Texture GetTexture(const Filepath& filepath, const SamplerDescription& samplerDescription);

    vk::Sampler GetSampler(const SamplerDescription& description);

private:
    std::shared_ptr<Device> device;
    std::shared_ptr<ImageManager> imageManager;

    std::unordered_map<Filepath, ImageHandle> textures;
    std::unordered_map<SamplerDescription, vk::Sampler> samplers;
};
