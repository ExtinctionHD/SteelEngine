#pragma once

class ComputePipeline;

struct Texture
{
    vk::Image image;
    vk::ImageView view;
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
};

namespace TextureHelpers
{
    constexpr uint32_t kCubeFaceCount = 6;
}

class PanoramaToCube
{
public:
    PanoramaToCube();
    ~PanoramaToCube();

    void Convert(const Texture& panoramaTexture, vk::Sampler panoramaSampler,
            vk::Image cubeImage, const vk::Extent2D& cubeImageExtent) const;

private:
    vk::DescriptorSetLayout panoramaLayout;
    vk::DescriptorSetLayout cubeFaceLayout;
    std::unique_ptr<ComputePipeline> pipeline;
};
