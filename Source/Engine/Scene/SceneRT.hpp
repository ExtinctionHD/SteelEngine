#pragma once

#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class Camera;
class SceneModel;

class SceneRT
{
public:
    struct Resources
    {
        std::vector<vk::AccelerationStructureKHR> accelerationStructures;
        std::vector<vk::Buffer> buffers;
        std::vector<vk::Sampler> samplers;
        std::vector<Texture> textures;
    };

    struct References
    {
        vk::AccelerationStructureKHR tlas;
        vk::Buffer cameraBuffer;
    };

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

    ~SceneRT();

    Camera *GetCamera() const { return camera.get(); }

    std::vector<vk::DescriptorSetLayout> GetDescriptorSetLayouts() const;

    std::vector<vk::DescriptorSet> GetDescriptorSets() const;

    void UpdateCameraBuffer(vk::CommandBuffer commandBuffer) const;

private:
    SceneRT(Camera *camera_, const Resources &resources_,
            const References &references_, const DescriptorSets &descriptorSets_);

    std::unique_ptr<Camera> camera;

    Resources resources;
    References references;
    DescriptorSets descriptorSets;

    friend class SceneModel;
};
