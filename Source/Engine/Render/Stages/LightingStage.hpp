#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

class Scene;
class ScenePT;
class Scene;
class Camera;
class Environment;
class ComputePipeline;
struct LightVolume;

class LightingStage
{
public:
    LightingStage(const Scene* scene_, const Camera* camera_,
            const Environment* environment_, const LightVolume* lightVolume_,
            const std::vector<vk::ImageView>& gBufferImageViews);

    ~LightingStage();

    void Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void Resize(const std::vector<vk::ImageView>& gBufferImageViews);

    void ReloadShaders();

private:
    struct LightingData
    {
        vk::Buffer directLightBuffer;
        DescriptorSet descriptorSet;
    };

    const Scene* scene = nullptr;
    const Camera* camera = nullptr;
    const Environment* environment = nullptr;
    const LightVolume* lightVolume = nullptr;

    DescriptorSet gBufferDescriptorSet;
    MultiDescriptorSet swapchainDescriptorSet;
    DescriptorSet rayTracingDescriptorSet;

    CameraData cameraData;
    LightingData lightingData;

    std::unique_ptr<ComputePipeline> pipeline;

    void SetupCameraData();

    void SetupLightingData();

    void SetupRayTracingData();

    void SetupPipeline();
};
