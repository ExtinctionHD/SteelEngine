#pragma once
#include "Engine/System/System.hpp"

class Scene;
class RenderPass;
class GraphicsPipeline;
struct KeyInput;

class RenderSystem
        : public System
{
public:
    RenderSystem(Scene* scene_);
    ~RenderSystem();

    void Process(float deltaSeconds) override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

private:
    struct DepthAttachment
    {
        vk::Image image;
        vk::ImageView view;
    };

    Scene* scene = nullptr;

    std::unique_ptr<RenderPass> forwardRenderPass;

    std::unique_ptr<GraphicsPipeline> defaultPipeline;
    std::unique_ptr<GraphicsPipeline> environmentPipeline;

    std::vector<DepthAttachment> depthAttachments;
    std::vector<vk::Framebuffer> framebuffers;

    void SetupDepthAttachments();

    void SetupFramebuffers();

    void HandleResizeEvent(const vk::Extent2D& extent);

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ReloadShaders();
};
