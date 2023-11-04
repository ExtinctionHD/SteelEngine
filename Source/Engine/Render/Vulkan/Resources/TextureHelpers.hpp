#pragma once

class ComputePipeline;
class DescriptorProvider;

// TODO rework image & texture naming, implement TextureCache
// struct BaseImage
//{
//    vk::Image image;
//    vk::ImageView view;
//};
//
// using RenderTarget = BaseImage;
//
// struct Texture
//{
//    vk::Image image;
//    vk::ImageView view;
//    vk::Sampler sampler;
//};

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
    std::unique_ptr<ComputePipeline> pipeline;

    std::unique_ptr<DescriptorProvider> descriptorProvider;
};

namespace TextureHelpers
{
    std::vector<vk::ImageView> GetViews(const std::vector<Texture>& textures);
}
