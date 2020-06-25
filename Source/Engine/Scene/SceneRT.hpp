#pragma once

#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class SceneModel;

class SceneRT
{
public:
    enum class DescriptorType
    {
        eGeneral,
        eIndices,
        ePositions,
        eNormals,
        eTangents,
        eTexCoords,
        eMaterials,
        eTextures,
    };

    using Descriptors = std::map<DescriptorType, DescriptorSet>;

private:
    vk::AccelerationStructureNV tlas;
    std::vector<Texture> textures;
    std::vector<vk::Sampler> samplers;
    std::vector<vk::Buffer> buffers;

    Descriptors descriptors;

    friend class SceneModel;
};
