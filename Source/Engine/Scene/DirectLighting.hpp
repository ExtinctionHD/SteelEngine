#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Shaders/Common/Common.h"

class DirectLighting
{
public:
    DirectLighting();
    ~DirectLighting();

    gpu::DirectLight RetrieveDirectLight(const Texture& panoramaTexture);

private:
    vk::DescriptorSetLayout storageImageLayout;
    vk::DescriptorSetLayout locationLayout;
    vk::DescriptorSetLayout parametersLayout;

    std::unique_ptr<ComputePipeline> luminancePipeline;
    std::unique_ptr<ComputePipeline> locationPipeline;
    std::unique_ptr<ComputePipeline> parametersPipeline;
};
