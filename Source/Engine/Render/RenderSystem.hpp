#pragma once

#include <memory>

#include "Engine/Render/Vulkan/VulkanContext.hpp"

class Window;

class RenderSystem
{
public:
    RenderSystem(const Window &window);
    ~RenderSystem();

    void Process() const;

    void Draw() const;

private:
    struct FrameData
    {
        vk::Framebuffer framebuffer;
        vk::CommandBuffer commandBuffer;
        vk::Semaphore presentCompleteSemaphore;
        vk::Semaphore renderCompleteSemaphore;
    };

    std::unique_ptr<VulkanContext> vulkanContext;

    std::vector<FrameData> frames;
};
