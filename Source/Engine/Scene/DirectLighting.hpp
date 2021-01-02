#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class DirectLighting
{
public:
    DirectLighting();
    ~DirectLighting();

    glm::vec3 RetrieveLightDirection(const Texture& panoramaTexture);

private:
    struct DescriptorSetLayouts
    {
        vk::DescriptorSetLayout panorama;
        vk::DescriptorSetLayout luminance;
        vk::DescriptorSetLayout location;
    };

    DescriptorSetLayouts layouts;

    std::unique_ptr<ComputePipeline> luminancePipeline;
    std::unique_ptr<ComputePipeline> locationPipeline;
};
