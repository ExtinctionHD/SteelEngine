#pragma once

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/RayTracing/RayTracingPipeline.hpp"
#include "Engine/Render/RenderObject.hpp"
#include "Engine/System.hpp"

class Window;

using RenderFunction = std::function<void(vk::CommandBuffer, uint32_t)>;

enum class RenderFlow
{
    eRasterization,
    eRayTracing
};

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

    struct RasterizationDescriptors
    {
        vk::DescriptorSetLayout layout;
        vk::DescriptorSet descriptorSet;
    };

    std::shared_ptr<VulkanContext> vulkanContext;

    std::unique_ptr<RenderPass> renderPass;
    std::unique_ptr<GraphicsPipeline> graphicsPipeline;
    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;

    bool drawingSuspended = true;

    RenderObject renderObject;

    Texture texture;
    RasterizationDescriptors rasterizationDescriptors;

    vk::AccelerationStructureNV tlas;
    RayTracingDescriptors rayTracingDescriptors;

    uint32_t frameIndex = 0;
    std::vector<FrameData> frames;
    std::vector<vk::Framebuffer> framebuffers;

    RenderFlow renderFlow = RenderFlow::eRasterization;

    RenderFunction mainRenderFunction;
    RenderFunction uiRenderFunction;

    vk::PipelineStageFlags presentCompleteWaitStages;

    void DrawFrame();

    void Rasterize(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void RayTrace(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void CreateRasterizationDescriptors();

    void CreateRayTracingDescriptors();

    void UpdateRayTracingDescriptors() const;
};
