#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Vulkan/VulkanHelpers.hpp"

class Scene;
class RayTracingPipeline;
struct KeyInput;

class PathTracingRenderer
{
public:
    PathTracingRenderer();

    virtual ~PathTracingRenderer();

    void RegisterScene(const Scene* scene_);

    void RemoveScene();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

    void Resize(const vk::Extent2D& extent);

protected:
    PathTracingRenderer(uint32_t sampleCount_, const vk::Extent2D& extent);

    virtual const CameraComponent& GetCameraComponent() const;

    struct RenderTargets
    {
        Texture accumulationTexture;
        MultiDescriptorSet descriptorSet;
        vk::Extent2D extent;
    };

    RenderTargets renderTargets;

private:
    const bool isProbeRenderer;
    const uint32_t sampleCount;

    const Scene* scene = nullptr;

    CameraData cameraData;
    DescriptorSet sceneDescriptorSet;

    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;
    std::unique_ptr<ComputePipeline> computePipeline;

    uint32_t accumulationIndex = 0;

    bool AccumulationEnabled() const { return !isProbeRenderer; }

    bool UseSwapchainRenderTarget() const { return !isProbeRenderer; }

    std::vector<vk::DescriptorSetLayout> GetDescriptorSetLayouts() const;

    void UpdateCameraBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ReloadShaders();

    void ResetAccumulation();
};
