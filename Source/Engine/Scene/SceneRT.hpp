#pragma once

#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class Camera;
class SceneModel;

class SceneRT
{
public:
    struct Info
    {
        uint32_t materialCount = 0;
    };

    struct Resources
    {
        std::vector<vk::AccelerationStructureKHR> accelerationStructures;
        std::vector<vk::Buffer> buffers;
        std::vector<vk::Sampler> samplers;
        std::vector<Texture> textures;
    };

    struct Description
    {
        Info info;
        Resources resources;
        DescriptorSet descriptorSet;
    };

    ~SceneRT();

    const Info& GetInfo() const { return description.info; }

    const DescriptorSet& GetDescriptorSet() const { return description.descriptorSet; }

private:
    SceneRT(const Description& description_);

    Description description;

    friend class SceneModel;
};
