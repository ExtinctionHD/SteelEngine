#pragma once

#include "Engine/Render/Vulkan/RayTracing/AccelerationStructureHelpers.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/System/System.hpp"

class SceneRT;
class Camera;
class RenderObject;
class RayTracingPipeline;

class PathTracingSystem
        : public System
{
public:
    PathTracingSystem(SceneRT *scene_);
    ~PathTracingSystem();

    void Process(float deltaSeconds) override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

private:
    struct AccumulationTarget
    {
        vk::Image image;
        vk::ImageView view;
        DescriptorSet descriptor;
        uint32_t currentIndex;
    };

    SceneRT *scene;

    MultiDescriptorSet renderTargets;
    AccumulationTarget accumulationTarget;

    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;

    void SetupRenderTargets();
    void SetupAccumulationTarget();

    void TraceRays(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

    void HandleResizeEvent(const vk::Extent2D &extent);

    void ResetAccumulation();
};
