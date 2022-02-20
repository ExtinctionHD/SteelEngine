#pragma once
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class ImageBasedLighting
{
public:
    struct Textures
    {
        Texture irradiance;
        Texture reflection;
    };

    struct Samplers
    {
        vk::Sampler irradiance;
        vk::Sampler reflection;
        vk::Sampler specularBRDF;
    };

    ImageBasedLighting();
    ~ImageBasedLighting();

    const Texture& GetSpecularBRDF() const { return specularBRDF; }

    const Samplers& GetSamplers() const { return samplers; }

    Textures GenerateTextures(const Texture& environmentTexture) const;

private:
    vk::DescriptorSetLayout environmentLayout;
    vk::DescriptorSetLayout targetLayout;
    vk::DescriptorSetLayout bufferLayout;

    std::unique_ptr<ComputePipeline> irradiancePipeline;
    std::unique_ptr<ComputePipeline> reflectionPipeline;

    Texture specularBRDF;

    Samplers samplers;
};
