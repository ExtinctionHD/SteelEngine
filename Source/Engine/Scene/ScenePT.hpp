#pragma once

#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Shaders/Common/Common.h"

class Camera;
class SceneModel;

class ScenePT
{
public:
    struct Info
    {
        uint32_t materialCount = 0;
        std::vector<PointLight> pointLights;
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

    ~ScenePT();

    const Info& GetInfo() const { return description.info; }

    const DescriptorSet& GetDescriptorSet() const { return description.descriptorSet; }

private:
    ScenePT(const Description& description_);

    Description description;

    friend class SceneModel;
};
