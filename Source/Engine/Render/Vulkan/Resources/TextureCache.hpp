#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Engine/Filesystem/Filepath.hpp"

struct ImageSource;

class TextureCache
{
public:
    TextureCache();

    Texture GetTexture(const Filepath& filepath) const;

    Texture CreateTexture(const ImageSource& source) const;

    Texture CreateColorTexture(const glm::vec4& color) const;

    Texture CreateCubeTexture(const BaseImage& panoramaImage, const vk::Extent2D& extent) const;
    
    vk::Sampler GetSampler(const SamplerDescription& description);

    vk::Sampler GetSampler(SamplerType type) const;

    void RemoveTextures(const Filepath& directory);

    void DestroyTexture(const Texture& texture);

private:
    PanoramaToCube panoramaToCube;

    std::map<Filepath, BaseImage> textureCache;

    std::map<SamplerDescription, vk::Sampler> samplerCache;
    std::map<SamplerType, vk::Sampler> defaultSamplerCache;
};
