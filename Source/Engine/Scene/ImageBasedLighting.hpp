#pragma once

#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

class ComputePipeline;

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

    const BaseImage& GetSpecularBRDF() const { return specularBRDF; }

    const Samplers& GetSamplers() const { return samplers; }

    BaseImage GenerateIrradianceImage(const BaseImage& cubemapImage) const;

    BaseImage GenerateReflectionImage(const BaseImage& cubemapImage) const;

private:
    std::unique_ptr<ComputePipeline> irradiancePipeline;
    std::unique_ptr<ComputePipeline> reflectionPipeline;

    BaseImage specularBRDF;

    Samplers samplers;
};
