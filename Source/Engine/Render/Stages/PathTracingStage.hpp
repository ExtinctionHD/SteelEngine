#pragma once

#include "Engine/Render/Stages/RenderStage.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

class Scene;
class RayTracingPipeline;
class DescriptorProvider;
struct KeyInput;

class PathTracingStage : public RenderStage
{
public:
    PathTracingStage(const SceneRenderContext& context_);

    ~PathTracingStage() override;

    void RegisterScene(const Scene* scene_) override;

    void UpdateResources() override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) override;

    void Resize() override;

    void ReloadShaders() override;

private:
    std::unique_ptr<RayTracingPipeline> pipeline;

    RenderTarget accumulationTarget;

    uint32_t accumulationIndex = 0;

    void UpdateDescriptors() const;

    void ResetAccumulation();
};
