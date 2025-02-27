#pragma once

#include "Engine/Render/Stages/RenderStage.hpp"

class Scene;
class ComputePipeline;
class DescriptorProvider;

class DebugDrawStage : public RenderStage
{
public:
    DebugDrawStage(const SceneRenderContext& context_);

    ~DebugDrawStage() override;

    void RegisterScene(const Scene* scene_) override;

    void RemoveScene() override;

    void Update() override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const override;

    void Resize() override;

    void ReloadShaders() override;

private:
    std::unique_ptr<ComputePipeline> pipeline;
    std::unique_ptr<DescriptorProvider> descriptorProvider;
};
