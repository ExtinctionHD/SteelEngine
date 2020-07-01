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
        eGeneral,
        eIndices,
        ePositions,
        eNormals,
        eTangents,
        eTexCoords,
        eTextures,
    };

    using Descriptors = std::map<DescriptorSetType, DescriptorSet>;

    Camera* GetCamera() const { return camera.get(); }

private:
    std::unique_ptr<Camera> camera;

    vk::AccelerationStructureNV tlas;

    std::vector<Texture> textures;
    std::vector<vk::Sampler> samplers;
    std::vector<vk::Buffer> buffers;

    Descriptors descriptors;

    friend class SceneModel;
};
