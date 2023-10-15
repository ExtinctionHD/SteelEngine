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

    CubeImage GenerateIrradianceImage(const CubeImage& cubemapImage) const;

    CubeImage GenerateReflectionImage(const CubeImage& cubemapImage) const;

private:
    std::unique_ptr<ComputePipeline> irradiancePipeline;
    std::unique_ptr<ComputePipeline> reflectionPipeline;

    BaseImage specularBRDF;

    Samplers samplers;
};
