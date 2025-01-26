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

    void RemoveScene() override;

    void Update() override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const override;

    void Resize() override;

    void ReloadShaders() override;

private:
    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;
    std::unique_ptr<DescriptorProvider> descriptorProvider;

    RenderTarget accumulationTarget;

    // TODO try to remove mutable

    mutable uint32_t accumulationIndex = 0;

    void ResetAccumulation() const;
};
