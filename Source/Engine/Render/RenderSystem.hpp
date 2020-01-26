#pragma once

#include <memory>

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"

class Window;

class RenderSystem
{
public:
    RenderSystem(const Window &window);
    ~RenderSystem();

    void Process() const;

    void Draw();

private:
    struct FrameData
    {
        vk::CommandBuffer commandBuffer;
        vk::Semaphore presentCompleteSemaphore;
        vk::Semaphore renderCompleteSemaphore;
        vk::Fence fence;
    };

    std::unique_ptr<VulkanContext> vulkanContext;
    std::unique_ptr<RenderPass> renderPass;
    std::unique_ptr<GraphicsPipeline> pipeline;

    BufferHandle vertexBuffer = nullptr;

    uint32_t frameIndex = 0;
    std::vector<FrameData> frames;
    std::vector<vk::Framebuffer> framebuffers;

    void DrawInternal(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};
