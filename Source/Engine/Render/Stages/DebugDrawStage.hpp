#pragma once

#include "Engine/Render/Stages/RenderStage.hpp"

class Scene;
class ComputePipeline;
class DescriptorProvider;

class DebugDrawStage : public RenderStage
{
public:
    DebugDrawStage(const SceneRenderContext& context_);

    void RemoveScene() override;

    void UpdateResources() override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) override;

    void Resize() override;

    void ReloadShaders() override;

private:
    std::unique_ptr<ComputePipeline> pipeline;

    void UpdateDescriptors() const;
};
