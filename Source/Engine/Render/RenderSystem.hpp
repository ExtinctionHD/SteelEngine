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
    RenderSystem(std::shared_ptr<VulkanContext> vulkanContext_, const RenderFunction &uiRenderFunction_);
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

    struct RayTracingDescriptors
    {
        vk::DescriptorSetLayout layout;
        std::vector<vk::DescriptorSet> descriptorSets;
    };

    std::shared_ptr<VulkanContext> vulkanContext;

    std::unique_ptr<RenderPass> renderPass;
    std::unique_ptr<GraphicsPipeline> graphicsPipeline;
    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;

    bool drawingSuspended = true;

    RenderObject renderObject;
    vk::AccelerationStructureNV tlas;
    RayTracingDescriptors rayTracingDescriptors;

    uint32_t frameIndex = 0;
    std::vector<FrameData> frames;
    std::vector<vk::Framebuffer> framebuffers;

    RenderFunction uiRenderFunction;

    void DrawFrame();

    void Rasterize(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void RayTrace(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void CreateRayTracingDescriptors();

    void UpdateRayTracingDescriptors() const;

    void UpdateSwapchainImageLayout(vk::CommandBuffer commandBuffer,
            uint32_t imageIndex, vk::ImageLayout layout) const;
};
