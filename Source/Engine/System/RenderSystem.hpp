#pragma once
#include "Engine/Scene/Scene.hpp"
#include "Engine/System/System.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

class Camera;
class Environment;
class RenderPass;
class GraphicsPipeline;
struct KeyInput;

class RenderSystem
        : public System
{
public:
    RenderSystem(Scene* scene_, Camera* camera_, Environment* environment_);
    ~RenderSystem() override;

    void Process(float deltaSeconds) override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

private:
    struct CameraData
    {
        vk::Buffer viewProjBuffer;
        vk::Buffer cameraPositionBuffer;
        DescriptorSet descriptorSet;
    };

    struct LightingData
    {
        vk::Buffer buffer;
        DescriptorSet descriptorSet;
    };

    struct EnvironmentData
    {
        vk::Buffer indexBuffer;
        vk::Buffer viewProjBuffer;
        DescriptorSet descriptorSet;
    };

    struct ScenePipeline
    {
        Scene::PipelineState state;
        std::unique_ptr<GraphicsPipeline> pipeline;
        std::vector<uint32_t> materialIndices;
    };

    struct DepthAttachment
    {
        vk::Image image;
        vk::ImageView view;
    };

    Scene* scene = nullptr;
    Camera* camera = nullptr;
    Environment* environment = nullptr;

    CameraData cameraData;
    LightingData lightingData;
    EnvironmentData environmentData;

    std::unique_ptr<RenderPass> forwardRenderPass;

    std::vector<ScenePipeline> scenePipelines;
    std::unique_ptr<GraphicsPipeline> environmentPipeline;

    std::vector<DepthAttachment> depthAttachments;
    std::vector<vk::Framebuffer> framebuffers;

    void SetupCameraData();
    void SetupLightingData();
    void SetupEnvironmentData();

    void SetupPipelines();
    void SetupDepthAttachments();
    void SetupFramebuffers();

    void UpdateCameraBuffers(vk::CommandBuffer commandBuffer) const;

    void DrawEnvironment(vk::CommandBuffer commandBuffer) const;

    void DrawScene(vk::CommandBuffer commandBuffer) const;

    void HandleResizeEvent(const vk::Extent2D& extent);

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ReloadShaders();
};
