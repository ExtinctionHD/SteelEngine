#pragma once

class ComputePipeline;
struct Texture;

class PanoramaToCubeConverter
{
public:
    PanoramaToCubeConverter();
    ~PanoramaToCubeConverter();

    void Convert(const Texture& panoramaTexture, vk::Sampler panoramaSampler,
            vk::Image cubeImage, const vk::Extent2D& cubeImageExtent) const;

private:
    vk::DescriptorSetLayout panoramaLayout;
    vk::DescriptorSetLayout cubeFaceLayout;
    std::unique_ptr<ComputePipeline> computePipeline;

    vk::DescriptorSet AllocatePanoramaDescriptorSet(const Texture& panoramaTexture, vk::Sampler panoramaSampler) const;
    std::vector<vk::DescriptorSet> AllocateCubeFacesDescriptorSets(vk::Image cubeImage) const;
};
