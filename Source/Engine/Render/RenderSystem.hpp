#pragma once

#include "Engine/System.hpp"
#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"
#include "Engine/Render/RenderObject.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Render/Rasterizer.hpp"

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
    RenderSystem(Scene &scene_, Camera &camera_,
            const RenderFunction &uiRenderFunction_);
    ~RenderSystem();

    void Process(float deltaSeconds) override;

    void OnResize(const vk::Extent2D &extent) override;

private:
    struct FrameData
    {
        vk::CommandBuffer commandBuffer;
        CommandBufferSync synchronization;
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

    std::unique_ptr<Renderer> renderer;

    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;

    bool drawingSuspended = true;

    Scene &scene;
    Camera &camera;

    Texture texture;
    RasterizationDescriptors rasterizationDescriptors;

    vk::AccelerationStructureNV blas;
    vk::AccelerationStructureNV tlas;
    RayTracingDescriptors rayTracingDescriptors;
    vk::Buffer rayTracingCameraBuffer;

    uint32_t frameIndex = 0;
    std::vector<FrameData> frames;

    RenderFlow renderFlow = RenderFlow::eRasterization;

    RenderFunction uiRenderFunction;

    void UpdateRayTracingResources(vk::CommandBuffer commandBuffer) const;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void RayTrace(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void CreateRasterizationDescriptors();

    void CreateRayTracingDescriptors();

    void UpdateRayTracingDescriptors() const;
};
