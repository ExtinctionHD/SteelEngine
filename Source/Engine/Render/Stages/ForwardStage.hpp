#pragma once

#include "Engine/Render/Stages/StageHelpers.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

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
    ForwardStage(Scene* scene_, Camera* camera_, Environment* environment_,
            LightVolume* lightVolume_, vk::ImageView depthImageView);
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
        uint32_t indexCount = 0;
        uint32_t instanceCount = 0;

        vk::Buffer indexBuffer;
        vk::Buffer vertexBuffer;
        vk::Buffer instanceBuffer;

        DescriptorSet descriptorSet;
    };

    Scene* scene = nullptr;
    Camera* camera = nullptr;
    Environment* environment = nullptr;
    LightVolume* lightVolume = nullptr;

    std::unique_ptr<RenderPass> renderPass;
    std::vector<vk::Framebuffer> framebuffers;

    CameraData defaultCameraData;
    CameraData environmentCameraData;

    EnvironmentData environmentData;
    PointLightsData pointLightsData;
    LightVolumeData lightVolumeData;

    std::unique_ptr<GraphicsPipeline> environmentPipeline;
    std::unique_ptr<GraphicsPipeline> pointLightsPipeline;
    std::unique_ptr<GraphicsPipeline> lightVolumePipeline;

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
