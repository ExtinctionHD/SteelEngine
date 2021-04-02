#pragma once

#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Engine/EngineHelpers.hpp"

class ScenePT;
class Camera;
class Environment;
class RayTracingPipeline;
struct KeyInput;

class PathTracer
{
public:
    PathTracer(ScenePT* scene_, Camera* camera_, Environment* environment_);
    ~PathTracer();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

private:
    struct RenderTargets
    {
        Texture accumulationTexture;
        MultiDescriptorSet descriptorSet;
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

    GeneralData generalData;

    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;
    std::unique_ptr<ComputePipeline> computePipeline;

    uint32_t accumulationIndex = 0;

    void SetupRenderTargets();

    void SetupGeneralData();

    void SetupPipeline();

    void UpdateCameraBuffer(vk::CommandBuffer commandBuffer) const;

    void HandleResizeEvent(const vk::Extent2D& extent);

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ReloadShaders();

    void ResetAccumulation();
};
