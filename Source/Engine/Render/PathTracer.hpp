#pragma once

#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Vulkan/VulkanHelpers.hpp"

class ScenePT;
class Camera;
class Environment;
class RayTracingPipeline;
struct KeyInput;

class PathTracer
{
public:
    PathTracer(ScenePT* scene_, Camera* camera_, Environment* environment_);
    virtual ~PathTracer();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

protected:
    PathTracer(ScenePT* scene_, Camera* camera_, Environment* environment_,
            uint32_t sampleCount_, const vk::Extent2D& extent);

    struct RenderTargets
    {
        Texture accumulationTexture;
        MultiDescriptorSet descriptorSet;
        vk::Extent2D extent;
    };

    RenderTargets renderTargets;

private:
    struct GeneralData
    {
        vk::Buffer cameraBuffer;
        vk::Buffer directLightBuffer;
        DescriptorSet descriptorSet;
    };

    const bool swapchainRenderTarget;
    const bool accumulationEnabled;
    const uint32_t sampleCount;

    ScenePT* scene = nullptr;
    Camera* camera = nullptr;
    Environment* environment = nullptr;

    GeneralData generalData;

    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;
    std::unique_ptr<ComputePipeline> computePipeline;

    uint32_t accumulationIndex = 0;

    void SetupRenderTargets(const vk::Extent2D& extent);

    void SetupGeneralData();

    void SetupPipeline();

    void UpdateCameraBuffer(vk::CommandBuffer commandBuffer) const;

    void HandleResizeEvent(const vk::Extent2D& extent);

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ReloadShaders();

    void ResetAccumulation();
};
