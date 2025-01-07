#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class ComputePipeline;

// TODO convert into LightingProbe
class ImageBasedLighting
{
public:
    ImageBasedLighting();
    ~ImageBasedLighting();

    const Texture& GetSpecularLut() const { return specularLut; }

    Texture GenerateIrradiance(const Texture& cubemap) const;

    Texture GenerateReflection(const Texture& cubemap) const;

private:
    std::unique_ptr<ComputePipeline> irradiancePipeline;
    std::unique_ptr<ComputePipeline> reflectionPipeline;

    Texture specularLut;
};
