#pragma once

#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/Systems/System.hpp"
#include "Engine/EngineHelpers.hpp"

class ScenePT;
class Camera;
class Environment;
class RayTracingPipeline;
struct KeyInput;

class RenderSystemPT
        : public System
{
public:
    RenderSystemPT(ScenePT* scene_, Camera* camera_, Environment* environment_);
    ~RenderSystemPT() override;

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
        vk::Buffer directLightBuffer;
        DescriptorSet descriptorSet;
    };

    ScenePT* scene = nullptr;
    Camera* camera = nullptr;
    Environment* environment = nullptr;

    RenderTargets renderTargets;
    AccumulationTarget accumulationTarget;

    GeneralData generalData;

    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;
    std::unique_ptr<ComputePipeline> computePipeline;

    void SetupRenderTargets();

    void SetupAccumulationTarget();

    void SetupGeneralData();

    void SetupPipeline();

    void UpdateCameraBuffer(vk::CommandBuffer commandBuffer) const;

    void HandleResizeEvent(const vk::Extent2D& extent);

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ReloadShaders();

    void ResetAccumulation();
};
