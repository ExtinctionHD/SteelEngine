#pragma once

#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class Camera;
class SceneModel;

class SceneRT
{
public:
    enum class DescriptorSetType
    {
        eTlas,
        eMaterials,
        eTextures,
        eIndices,
        ePositions,
        eNormals,
        eTangents,
        eTexCoords,
    };

    using DescriptorSets = std::map<DescriptorSetType, DescriptorSet>;

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
        DescriptorSets descriptorSets;
    };

    ~SceneRT();

    const Info& GetInfo() const { return description.info; }

    std::vector<vk::DescriptorSetLayout> GetDescriptorSetLayouts() const;

    std::vector<vk::DescriptorSet> GetDescriptorSets() const;

private:
    SceneRT(const Description& description_);

    Description description;

    friend class SceneModel;
};
