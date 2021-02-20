#pragma once

#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class Camera;
class SceneModel;

class ScenePT
{
public:
    struct Info
    {
        uint32_t materialCount = 0;
        uint32_t pointLightCount = 0;
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
        std::vector<DescriptorSet> descriptorSets;
    };

    ~ScenePT();

    const Info& GetInfo() const { return description.info; }

    const std::vector<DescriptorSet>& GetDescriptorSets() const { return description.descriptorSets; }

private:
    ScenePT(const Description& description_);

    Description description;

    friend class SceneModel;
};
