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
        eTextures,
        eIndices,
        ePositions,
        eNormals,
        eTangents,
        eTexCoords,
    };

    using DescriptorSets = std::map<DescriptorSetType, DescriptorSet>;

    Camera* GetCamera() const { return camera.get(); }

    std::vector<vk::DescriptorSetLayout> GetDescriptorSetLayouts() const;

    std::vector<vk::DescriptorSet> GetDescriptorSets() const;

    void UpdateCameraBuffer(vk::CommandBuffer commandBuffer) const;

private:
    std::unique_ptr<Camera> camera;

    vk::AccelerationStructureNV tlas;
    std::vector<vk::AccelerationStructureNV> blases;

    std::vector<Texture> textures;
    std::vector<vk::Sampler> samplers;
    std::vector<vk::Buffer> buffers;

    DescriptorSets descriptorSets;

    friend class SceneModel;
};
