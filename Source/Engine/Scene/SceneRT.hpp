#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class SceneModel;

class SceneRT
{
public:
    enum class GeometryBufferType
    {
        eIndices,
        ePositions,
        eNormals,
        eTangents,
        eTexCoords,
    };

    using GeometryBuffers = std::map<GeometryBufferType, std::vector<vk::Buffer>>;

private:
    vk::AccelerationStructureNV tlas;
    GeometryBuffers geometryBuffers;
    std::vector<Texture> textures;
    std::vector<vk::Sampler> samplers;

    friend class SceneModel;
};
