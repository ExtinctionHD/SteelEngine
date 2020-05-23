#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Engine/Filesystem/Filepath.hpp"

class TextureManager
{
public:
    TextureManager(const SamplerDescription &defaultSamplerDescription);
    ~TextureManager();

    Texture CreateTexture(const Filepath &filepath) const;

    Texture CreateColorTexture(const glm::vec3 &color) const;

    Texture CreateCubeTexture(const Texture &panoramaTexture, const vk::Extent2D &extent) const;

    vk::Sampler CreateSampler(const SamplerDescription &description) const;

    vk::Sampler GetDefaultSampler() const { return defaultSampler; }

    void DestroyTexture(const Texture &texture) const;

private:
    vk::Sampler defaultSampler;
};
