#include "Engine/Render/Vulkan/Resources/TextureCache.hpp"

#include "Engine/Render/Vulkan/Resources/ImageManager.hpp"

namespace STextureCache
{}

TextureCache::TextureCache(std::shared_ptr<Device> aDevice, std::shared_ptr<ImageManager> aImageManager)
    : device(aDevice)
    , imageManager(aImageManager)
{}

TextureCache::~TextureCache()
{
    for (auto &[description, sampler] : samplers)
    {
        device->Get().destroySampler(sampler);
    }

    for (auto &[filepath, image] : textures)
    {
        imageManager->DestroyImage(image);
    }
}

Texture TextureCache::GetTexture(const Filepath &filepath, const SamplerDescription &samplerDescription)
{}

vk::Sampler TextureCache::GetSampler(const SamplerDescription &description)
{
    const auto it = samplers.find(description);

    if (it != samplers.end())
    {
        return it->second;
    }

    const vk::SamplerCreateInfo createInfo({},
            description.magFilter, description.minFilter,
            description.mipmapMode, description.addressMode,
            description.addressMode, description.addressMode, 0.0f,
            description.maxAnisotropy.has_value(),
            description.maxAnisotropy.value_or(0.0f),
            false, {}, description.minLod, description.maxLod,
            vk::BorderColor::eFloatOpaqueBlack, false);

    const auto &[result, sampler] = device->Get().createSampler(createInfo);
    Assert(result == vk::Result::eSuccess);

    samplers.emplace(description, sampler);

    return sampler;
}
