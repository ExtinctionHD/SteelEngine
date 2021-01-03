#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Shaders/Common/Common.h"

class DirectLighting
{
public:
    DirectLighting();
    ~DirectLighting();

    DirectLight RetrieveDirectLight(const Texture& panoramaTexture);

private:
    struct DescriptorSetLayouts
    {
        vk::DescriptorSetLayout panorama;
        vk::DescriptorSetLayout luminance;
        vk::DescriptorSetLayout location;
        vk::DescriptorSetLayout parameters;
    };

    DescriptorSetLayouts layouts;

    std::unique_ptr<ComputePipeline> luminancePipeline;
    std::unique_ptr<ComputePipeline> locationPipeline;
    std::unique_ptr<ComputePipeline> parametersPipeline;
};
