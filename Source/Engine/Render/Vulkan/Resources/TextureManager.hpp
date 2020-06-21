#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Engine/Filesystem/Filepath.hpp"

#include "Utils/DataHelpers.hpp"

class TextureManager
{
public:
    TextureManager(const SamplerDescription &defaultSamplerDescription);
    ~TextureManager();

    Texture CreateTexture(const Filepath &filepath) const;

    Texture CreateTexture(vk::Format format, const vk::Extent2D &extent, const ByteView &data) const;

    Texture CreateCubeTexture(const Texture &panoramaTexture, const vk::Extent2D &extent) const;

    Texture CreateColorTexture(const glm::vec4 &color) const;

    vk::Sampler CreateSampler(const SamplerDescription &description) const;

    vk::Sampler GetDefaultSampler() const { return defaultSampler; }

    void DestroyTexture(const Texture &texture) const;

    void DestroySampler(vk::Sampler sampler) const;

private:
    vk::Sampler defaultSampler;
};
