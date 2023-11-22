#pragma once

#include "ImageHelpers.hpp"

class ComputePipeline;
class DescriptorProvider;

struct Texture
{
    BaseImage image;
    vk::Sampler sampler;
};

enum class DefaultTexture
{
    eBlack,
    eWhite,
    eNormal,
    eCheckered,
};

enum class DefaultSampler
{
    eLinearRepeat,
    eTexelClamp,
};

struct SamplerDescription
{
    vk::Filter magFilter = vk::Filter::eLinear;
    vk::Filter minFilter = vk::Filter::eLinear;
    vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear;
    vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat;
    float maxAnisotropy = 16.0f;
    float minLod = 0.0f;
    float maxLod = std::numeric_limits<float>::max();
    bool unnormalizedCoords = false;

    bool operator==(const SamplerDescription& other) const;
    bool operator<(const SamplerDescription& other) const;
};

class PanoramaToCube
{
public:
    PanoramaToCube();
    ~PanoramaToCube();

    CubeImage GenerateCubeImage(const BaseImage& panoramaImage,
            vk::ImageUsageFlags usage, vk::ImageLayout finalLayout) const;

private:
    std::unique_ptr<ComputePipeline> pipeline;

    std::unique_ptr<DescriptorProvider> descriptorProvider;
};
