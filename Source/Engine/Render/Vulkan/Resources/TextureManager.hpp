#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Engine/Filesystem/Filepath.hpp"

#include "Utils/DataHelpers.hpp"

class ComputePipeline;

class TextureManager
{
public:
    TextureManager() = default;

    Texture CreateTexture(const Filepath& filepath) const;

    Texture CreateTexture(vk::Format format, const vk::Extent2D& extent, const ByteView& data) const;

    Texture CreateCubeTexture(const Texture& panoramaTexture, const vk::Extent2D& extent) const;

    Texture CreateColorTexture(const glm::vec4& color) const;

    vk::Sampler CreateSampler(const SamplerDescription& description) const;

    void DestroyTexture(const Texture& texture) const;

    void DestroySampler(vk::Sampler sampler) const;

private:
    PanoramaToCube panoramaToCube;
};
