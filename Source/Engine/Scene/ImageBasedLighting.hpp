#pragma once
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class ImageBasedLighting
{
public:
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
    
    Texture GenerateIrradianceTexture(const Texture& cubemapTexture) const;

    Texture GenerateReflectionTexture(const Texture& cubemapTexture) const;

private:
    vk::DescriptorSetLayout cubemapLayout;
    vk::DescriptorSetLayout targetLayout;

    std::unique_ptr<ComputePipeline> irradiancePipeline;
    std::unique_ptr<ComputePipeline> reflectionPipeline;

    Texture specularBRDF;

    Samplers samplers;
};
