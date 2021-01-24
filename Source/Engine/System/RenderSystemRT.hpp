#pragma once

#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/System/System.hpp"

class SceneRT;
class Camera;
class Environment;
class RayTracingPipeline;
struct KeyInput;

class RenderSystemRT
        : public System
{
public:
    RenderSystemRT(SceneRT* scene_, Camera* camera_, Environment* environment_);
    ~RenderSystemRT() override;

    void Process(float deltaSeconds) override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

private:
    struct RenderTargets
    {
        MultiDescriptorSet descriptorSet;
    };

    struct AccumulationTarget
    {
        vk::Image image;
        vk::ImageView view;
        DescriptorSet descriptorSet;
        uint32_t accumulationCount = 0;
    };

    struct GeneralData
    {
        vk::Buffer cameraBuffer;
        vk::Buffer lightingBuffer;
        DescriptorSet descriptorSet;
    };

    SceneRT* scene = nullptr;
    Camera* camera = nullptr;
    Environment* environment = nullptr;

    RenderTargets renderTargets;
    AccumulationTarget accumulationTarget;

    GeneralData generalData;

    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;

    void SetupRenderTargets();
    void SetupAccumulationTarget();

    void SetupGeneralData();

    void SetupRayTracingPipeline();

    void UpdateCameraBuffer(vk::CommandBuffer commandBuffer) const;

    void HandleResizeEvent(const vk::Extent2D& extent);

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ReloadShaders();

    void ResetAccumulation();
};
