#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

class Scene;
class ComputePipeline;

class LightingStage
{
public:
    LightingStage(const std::vector<RenderTarget>& gBufferTargets_);

    ~LightingStage();

    void RegisterScene(const Scene* scene_);

    void RemoveScene();

    void Update() const;

    void Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void Resize(const std::vector<RenderTarget>& gBufferTargets_);

    void ReloadShaders();

private:
    const Scene* scene = nullptr;

    std::vector<RenderTarget> gBufferTargets;

    std::unique_ptr<ComputePipeline> pipeline;

    std::unique_ptr<DescriptorProvider> descriptorProvider;
};
