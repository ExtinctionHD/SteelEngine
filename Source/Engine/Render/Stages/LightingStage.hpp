#pragma once

#include "Engine/Render/Stages/StageHelpers.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Scene/GlobalIllumination.hpp"

class ScenePT;
class Scene;
class Camera;
class Environment;
class ComputePipeline;

class LightingStage
{
public:
    LightingStage(Scene* scene_, ScenePT* scenePT_, Camera* camera_, Environment* environment_,
            const std::vector<vk::ImageView>& gBufferImageViews);

    ~LightingStage();

    void Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void Resize(const std::vector<vk::ImageView>& gBufferImageViews);

    void ReloadShaders();

private:
    struct LightingData
    {
        IrradianceVolume irradianceVolume;
        vk::Buffer directLightBuffer;
        DescriptorSet descriptorSet;
    };

    Scene* scene = nullptr;
    ScenePT* scenePT = nullptr;
    Camera* camera = nullptr;
    Environment* environment = nullptr;

    DescriptorSet gBufferDescriptorSet;
    MultiDescriptorSet swapchainDescriptorSet;

    CameraData cameraData;
    LightingData lightingData;

    std::unique_ptr<ComputePipeline> pipeline;

    void SetupCameraData();

    void SetupLightingData();

    void SetupPipeline();
};
