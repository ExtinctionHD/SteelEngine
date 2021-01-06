#pragma once
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class ImageBasedLighting
{
public:
    ImageBasedLighting();
    ~ImageBasedLighting();

    Texture GenerateIrradianceTexture(const Texture& environmentTexture, vk::Sampler environmentSampler) const;

private:
    vk::DescriptorSetLayout environmentLayout;
    vk::DescriptorSetLayout targetLayout;

    std::unique_ptr<ComputePipeline> irradiancePipeline;
};
