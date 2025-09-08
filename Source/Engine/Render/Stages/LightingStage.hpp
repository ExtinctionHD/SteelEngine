#pragma once

#include "Engine/Render/Stages/RenderStage.hpp"

class ComputePipeline;
class DescriptorProvider;

class LightingStage : public RenderStage
{
public:
    LightingStage(const SceneRenderContext& context_);

    void RegisterScene(const Scene* scene_) override;

    void UpdateResources() override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) override;

    void Resize() override;

    void ReloadShaders() override;

private:
    void UpdateDescriptors() const;

    std::unique_ptr<ComputePipeline> pipeline;
};
