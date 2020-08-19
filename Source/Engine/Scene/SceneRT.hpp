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

    struct References
    {
        vk::AccelerationStructureKHR tlas;
        vk::Buffer cameraBuffer;
    };

    struct Description
    {
        Info info;
        Resources resources;
        References references;
        DescriptorSets descriptorSets;
    };

    ~SceneRT();

    Camera* GetCamera() const { return camera.get(); }

    const Info& GetInfo() const { return description.info; }

    std::vector<vk::DescriptorSetLayout> GetDescriptorSetLayouts() const;

    std::vector<vk::DescriptorSet> GetDescriptorSets() const;

    void UpdateCameraBuffer(vk::CommandBuffer commandBuffer) const;

private:
    SceneRT(Camera* camera_, const Description& description_);

    std::unique_ptr<Camera> camera;

    Description description;

    friend class SceneModel;
};
