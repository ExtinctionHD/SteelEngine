#pragma once

#include "ImageHelpers.hpp"

class ComputePipeline;
class DescriptorProvider;

struct ViewSampler // TODO rename to Texture
{
    vk::ImageView view;
    vk::Sampler sampler;
};

struct SamplerDescription
{
    vk::Filter magFilter;
    vk::Filter minFilter;
    vk::SamplerMipmapMode mipmapMode;
    vk::SamplerAddressMode addressMode;
    std::optional<float> maxAnisotropy;
    float minLod;
    float maxLod;
    bool unnormalizedCoords;
};

class PanoramaToCube
{
public:
    PanoramaToCube();
    ~PanoramaToCube();

    CubeImage GenerateCubeImage(const BaseImage& panoramaImage, const vk::Extent2D& extent,
            vk::ImageUsageFlags usage, vk::ImageLayout finalLayout) const;

private:
    std::unique_ptr<ComputePipeline> pipeline;

    std::unique_ptr<DescriptorProvider> descriptorProvider;
};
