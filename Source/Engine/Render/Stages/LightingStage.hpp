#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorHelpers.hpp"

class Scene;
class ComputePipeline;

class LightingStage
{
public:
    LightingStage(const std::vector<vk::ImageView>& gBufferImageViews);

    ~LightingStage();

    void RegisterScene(const Scene* scene_);

    void RemoveScene();

    void Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void Resize(const std::vector<vk::ImageView>& gBufferImageViews);

    void ReloadShaders();

private:
    const Scene* scene = nullptr;

    MultiDescriptorSet swapchainDescriptorSet;
    DescriptorSet gBufferDescriptorSet;
    DescriptorSet lightingDescriptorSet;
    DescriptorSet rayTracingDescriptorSet;

    CameraData cameraData;

    std::unique_ptr<ComputePipeline> pipeline;

    std::vector<vk::DescriptorSetLayout> GetDescriptorSetLayouts() const;
};
