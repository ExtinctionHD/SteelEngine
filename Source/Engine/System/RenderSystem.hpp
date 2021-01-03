#pragma once
#include "Engine/System/System.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

class Scene;
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
    struct DefaultData
    {
        vk::Buffer viewProjBuffer;
        vk::Buffer cameraPositionBuffer;
        vk::Buffer directLightBuffer;
        DescriptorSet descriptorSet;
    };

    struct EnvironmentData
    {
        vk::Buffer indexBuffer;
        DescriptorSet descriptorSet;
    };

    struct DepthAttachment
    {
        vk::Image image;
        vk::ImageView view;
    };

    Scene* scene = nullptr;
    Camera* camera = nullptr;
    Environment* environment = nullptr;

    DefaultData defaultData;
    EnvironmentData environmentData;

    std::unique_ptr<RenderPass> forwardRenderPass;

    std::unique_ptr<GraphicsPipeline> defaultPipeline;
    std::unique_ptr<GraphicsPipeline> environmentPipeline;

    std::vector<DepthAttachment> depthAttachments;
    std::vector<vk::Framebuffer> framebuffers;

    void SetupDefaultData();
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
