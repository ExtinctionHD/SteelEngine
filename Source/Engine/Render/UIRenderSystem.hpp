#pragma once

#include "Engine/System.hpp"
#include "Engine/Render/RenderSystem.hpp"

class UIRenderSystem
        : public System
{
public:
    UIRenderSystem(std::shared_ptr<VulkanContext> aVulkanContext, const Window &window);
    ~UIRenderSystem();

    void Process(float deltaTime) override;

    void OnResize(const vk::Extent2D &extent) override;

    RenderFunction GetUIRenderFunction();

private:
    std::shared_ptr<VulkanContext> vulkanContext;

    vk::DescriptorPool descriptorPool;
    std::unique_ptr<RenderPass> renderPass;

    std::vector<vk::Framebuffer> framebuffers;

    void RenderUI(vk::CommandBuffer commandBuffer, uint32_t imageIndex);
};
