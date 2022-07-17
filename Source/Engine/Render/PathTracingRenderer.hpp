#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Vulkan/VulkanHelpers.hpp"

class Scene;
class RayTracingPipeline;
struct KeyInput;

class PathTracingRenderer
{
public:
    PathTracingRenderer(const Scene* scene_);

    virtual ~PathTracingRenderer();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

protected:
    PathTracingRenderer(const Scene* scene_,
            uint32_t sampleCount_, const vk::Extent2D& extent);

    virtual const CameraComponent& GetCameraComponent() const;

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

    const Scene* scene = nullptr;

    CameraData cameraData;
    GeneralData generalData;
    DescriptorSet sceneDescriptorSet;

    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;
    std::unique_ptr<ComputePipeline> computePipeline;

    uint32_t accumulationIndex = 0;

    bool AccumulationEnabled() const { return !isProbeRenderer; }

    bool UseSwapchainRenderTarget() const { return !isProbeRenderer; }

    void SetupRenderTargets(const vk::Extent2D& extent);

    void SetupCameraData(uint32_t bufferCount);

    void SetupGeneralData();

    void SetupSceneData();

    void SetupPipeline();

    void UpdateCameraBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void HandleResizeEvent(const vk::Extent2D& extent);

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ReloadShaders();

    void ResetAccumulation();
};
