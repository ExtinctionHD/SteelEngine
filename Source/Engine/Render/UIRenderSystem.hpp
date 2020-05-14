#pragma once

#include "Engine/System.hpp"

class Window;
class RenderPass;

class UIRenderSystem
        : public System
{
public:
    UIRenderSystem(const Window &window);
    ~UIRenderSystem();

    void Process(float deltaSeconds) override;

    void OnResize(const vk::Extent2D &extent) override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

private:
    vk::DescriptorPool descriptorPool;
    std::unique_ptr<RenderPass> renderPass;

    std::vector<vk::Framebuffer> framebuffers;
};
