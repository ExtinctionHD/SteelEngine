#pragma once
#include "Engine/Scene/Scene.hpp"
#include "Engine/Systems/System.hpp"
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
        std::vector<RenderTarget> textures;
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
        std::vector<vk::Buffer> cameraBuffers;
        MultiDescriptorSet descriptorSet;
    };

    struct LightingData
    {
        std::vector<vk::Buffer> cameraBuffers;
        vk::Buffer directLightBuffer;
        MultiDescriptorSet descriptorSet;
    };

    struct EnvironmentData
    {
        vk::Buffer indexBuffer;
        std::vector<vk::Buffer> cameraBuffers;
        MultiDescriptorSet descriptorSet;
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

    void UpdateCameraBuffers(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void ExecuteGBufferRenderPass(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void ComputeLighting(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void ExecuteForwardRenderPass(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void DrawScene(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void DrawEnvironment(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void DrawPointLights(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void HandleResizeEvent(const vk::Extent2D& extent);
    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ReloadShaders();
};
