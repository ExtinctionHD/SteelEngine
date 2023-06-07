#pragma once

class ComputePipeline;

struct Texture
{
    vk::Image image;
    vk::ImageView view;
};

struct TextureSampler
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

    void Convert(const Texture& panoramaTexture,
            vk::Image cubeImage, const vk::Extent2D& cubeImageExtent) const;

private:
    vk::DescriptorSetLayout panoramaLayout;
    vk::DescriptorSetLayout cubeFaceLayout;
    std::unique_ptr<ComputePipeline> pipeline;
};

namespace TextureHelpers
{
    std::vector<vk::ImageView> GetViews(const std::vector<Texture>& textures);
}
