#pragma once
#include "Engine/Scene/Scene.hpp"
#include "Engine/System/System.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

class Camera;
class Environment;
class RenderPass;
class ComputePipeline;
class GraphicsPipeline;
struct KeyInput;

class RenderSystem
        : public System
{
public:
    RenderSystem(Scene* scene_, Camera* camera_, Environment* environment_);
    ~RenderSystem() override;

    void Process(float deltaSeconds) override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

private:
    struct GBufferData
    {
        RenderTarget normals;
        RenderTarget baseColor;
        RenderTarget emission;
        RenderTarget misc;
        RenderTarget depth;

        DescriptorSet descriptorSet;
        vk::Framebuffer framebuffer;
    };

    struct SwapchainData
    {
        MultiDescriptorSet descriptorSet;
        std::vector<vk::Framebuffer> framebuffers;
    };

    struct CameraData
    {
        vk::Buffer viewProjBuffer;
        DescriptorSet descriptorSet;
    };

    struct LightingData
    {
        vk::Buffer directLightBuffer;
        DescriptorSet descriptorSet;
    };

    struct EnvironmentData
    {
        vk::Buffer indexBuffer;
        vk::Buffer viewProjBuffer;
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

    struct ScenePipeline
    {
        Scene::PipelineState state;
        std::unique_ptr<GraphicsPipeline> pipeline;
        std::vector<uint32_t> materialIndices;
    };

    Scene* scene = nullptr;
    Camera* camera = nullptr;
    Environment* environment = nullptr;

    GBufferData gBufferData;
    SwapchainData swapchainData;

    CameraData cameraData;
    LightingData lightingData;
    EnvironmentData environmentData;
    PointLightsData pointLightsData;

    std::unique_ptr<RenderPass> gBufferRenderPass;
    std::unique_ptr<RenderPass> forwardRenderPass;

    std::vector<ScenePipeline> scenePipelines;
    std::unique_ptr<ComputePipeline> lightingPipeline;
    std::unique_ptr<GraphicsPipeline> environmentPipeline;
    std::unique_ptr<GraphicsPipeline> pointLightsPipeline;

    void SetupGBufferData();
    void SetupSwapchainData();

    void SetupCameraData();
    void SetupLightingData();
    void SetupEnvironmentData();
    void SetupPointLightsData();

    void SetupRenderPasses();
    void SetupFramebuffers();
    void SetupPipelines();

    void UpdateCameraBuffers(vk::CommandBuffer commandBuffer) const;

    void ExecuteGBufferRenderPass(vk::CommandBuffer commandBuffer) const;

    void ComputeLighting(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void ExecuteForwardRenderPass(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void DrawScene(vk::CommandBuffer commandBuffer) const;

    void DrawEnvironment(vk::CommandBuffer commandBuffer) const;

    void DrawPointLights(vk::CommandBuffer commandBuffer) const;

    void HandleResizeEvent(const vk::Extent2D& extent);
    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ReloadShaders();
};
