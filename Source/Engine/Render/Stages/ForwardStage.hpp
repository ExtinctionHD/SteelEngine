#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

class Scene;
class Scene;
class Camera;
class Environment;
class RenderPass;
class GraphicsPipeline;
struct LightVolume;
struct KeyInput;

class ForwardStage
{
public:
    ForwardStage(const Scene* scene_, const Camera* camera_,
            const Environment* environment_, const LightVolume* lightVolume_,
            vk::ImageView depthImageView);

    ~ForwardStage();

    void Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void Resize(vk::ImageView depthImageView);

    void ReloadShaders();

private:
    struct EnvironmentData
    {
        vk::Buffer indexBuffer;
        DescriptorSet descriptorSet;
    };

    struct PointLightsData
    {
        uint32_t indexCount = 0;
        uint32_t instanceCount = 0;

        vk::Buffer indexBuffer;
        vk::Buffer vertexBuffer;
        vk::Buffer instanceBuffer;
    };

    struct LightVolumeData
    {
        uint32_t positionsIndexCount = 0;
        uint32_t positionsInstanceCount = 0;
        uint32_t edgesIndexCount = 0;

        vk::Buffer positionsIndexBuffer;
        vk::Buffer positionsVertexBuffer;
        vk::Buffer positionsInstanceBuffer;
        vk::Buffer edgesIndexBuffer;

        DescriptorSet positionsDescriptorSet;
    };

    const Scene* scene = nullptr;
    const Camera* camera = nullptr;
    const Environment* environment = nullptr;
    const LightVolume* lightVolume = nullptr;

    std::unique_ptr<RenderPass> renderPass;
    std::vector<vk::Framebuffer> framebuffers;

    CameraData defaultCameraData;
    CameraData environmentCameraData;

    EnvironmentData environmentData;
    PointLightsData pointLightsData;
    LightVolumeData lightVolumeData;

    std::unique_ptr<GraphicsPipeline> environmentPipeline;
    std::unique_ptr<GraphicsPipeline> pointLightsPipeline;
    std::unique_ptr<GraphicsPipeline> lightVolumePositionsPipeline;
    std::unique_ptr<GraphicsPipeline> lightVolumeEdgesPipeline;

    bool drawLightVolume = false;

    void SetupCameraData();
    void SetupEnvironmentData();
    void SetupPointLightsData();
    void SetupLightVolumeData();

    void DrawEnvironment(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
    void DrawPointLights(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
    void DrawLightVolume(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void SetupPipelines();

    void HandleKeyInputEvent(const KeyInput& keyInput);
};
