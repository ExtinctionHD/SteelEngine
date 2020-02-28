#pragma once

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/RayTracing/RayTracingPipeline.hpp"
#include "Engine/Render/RenderObject.hpp"
#include "Engine/System.hpp"

class Window;

using RenderFunction = std::function<void(vk::CommandBuffer, uint32_t)>;

class RenderSystem
        : public System
{
public:
    RenderSystem(std::shared_ptr<VulkanContext> aVulkanContext, const RenderFunction &aUIRenderFunction);
    ~RenderSystem();

    void Process(float timeElapsed) override;

    void OnResize(const vk::Extent2D &extent) override;

private:
    struct FrameData
    {
        vk::CommandBuffer commandBuffer;
        vk::Semaphore presentCompleteSemaphore;
        vk::Semaphore renderCompleteSemaphore;
        vk::Fence fence;
    };

    std::shared_ptr<VulkanContext> vulkanContext;

    std::unique_ptr<RenderPass> renderPass;
    std::unique_ptr<GraphicsPipeline> graphicsPipeline;
    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;

    BufferHandle shaderBindingTable;

    bool drawingSuspended = true;

    RenderObject renderObject;

    vk::AccelerationStructureNV tlas;

    uint32_t frameIndex = 0;
    std::vector<FrameData> frames;
    std::vector<vk::Framebuffer> framebuffers;

    RenderFunction uiRenderFunction;

    void DrawFrame();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};
