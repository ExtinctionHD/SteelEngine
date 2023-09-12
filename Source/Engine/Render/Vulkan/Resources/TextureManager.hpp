#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Engine/Filesystem/Filepath.hpp"

#include "Utils/DataHelpers.hpp"

// TODO rework
class TextureManager
{
public:
    TextureManager() = default;

    BaseImage CreateTexture(const Filepath& filepath) const;

    BaseImage CreateTexture(vk::Format format, const vk::Extent2D& extent, const ByteView& data) const;

    BaseImage CreateCubeTexture(const BaseImage& panoramaTexture, const vk::Extent2D& extent) const;

    BaseImage CreateColorTexture(const glm::vec4& color) const;

    vk::Sampler CreateSampler(const SamplerDescription& description) const;

    void DestroyTexture(const BaseImage& texture) const;

    void DestroySampler(vk::Sampler sampler) const;

private:
    PanoramaToCube panoramaToCube;
};
