#pragma once

#include "Engine/Systems/System.hpp"

class Window;
class RenderPass;

// TODO rename to UIRenderer
class UIRenderSystem
        : public System
{
public:
    using TextBinding = std::function<std::string()>;

    UIRenderSystem(const Window& window);
    ~UIRenderSystem() override;

    void Process(Scene& scene, float deltaSeconds) override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

    void BindText(const TextBinding& textBinding);

private:
    vk::DescriptorPool descriptorPool;
    std::unique_ptr<RenderPass> renderPass;

    std::vector<vk::Framebuffer> framebuffers;

    std::vector<TextBinding> textBindings;

    void HandleResizeEvent(const vk::Extent2D& extent);
};
