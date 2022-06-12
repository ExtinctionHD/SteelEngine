#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Vulkan/VulkanHelpers.hpp"

class ScenePT;
class Camera;
class Environment;
class RayTracingPipeline;
struct KeyInput;

class PathTracingRenderer
{
public:
    PathTracingRenderer(const ScenePT* scene_,
            const Camera* camera_, const Environment* environment_);

    virtual ~PathTracingRenderer();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

protected:
    PathTracingRenderer(const ScenePT* scene_,
            const Camera* camera_, const Environment* environment_,
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
        vk::Buffer directLightBuffer;
        DescriptorSet descriptorSet;
    };

    const bool isProbeRenderer;
    const uint32_t sampleCount;

    const ScenePT* scene = nullptr;
    const Camera* camera = nullptr;
    const Environment* environment = nullptr;

    CameraData cameraData;
    GeneralData generalData;

    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;
    std::unique_ptr<ComputePipeline> computePipeline;

    uint32_t accumulationIndex = 0;

    bool AccumulationEnabled() const { return !isProbeRenderer; }

    bool UseSwapchainRenderTarget() const { return !isProbeRenderer; }

    void SetupRenderTargets(const vk::Extent2D& extent);

    void SetupCameraData(uint32_t bufferCount);

    void SetupGeneralData();

    void SetupPipeline();

    void UpdateCameraBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void HandleResizeEvent(const vk::Extent2D& extent);

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ReloadShaders();

    void ResetAccumulation();
};