#pragma once

#include "Engine/Render/Stages/RenderStage.hpp"
#include "Engine/Scene/Material.hpp"

class GraphicsPipeline;
class Scene;
class RenderPass;
class MaterialPipelineCache;

// TODO rename to ForwardStage
class TranslucentStage : public RenderStage
{
public:
    TranslucentStage(const SceneRenderContext& context_);

    ~TranslucentStage() override;

    void RegisterScene(const Scene* scene_) override;

    void UpdateResources() override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) override;

    void Resize() override;

    void ReloadShaders() override;

private:
    std::unique_ptr<RenderPass> renderPass;
    std::unique_ptr<MaterialPipelineCache> pipelineCache;
    std::set<MaterialFlags> uniquePipelines;

    std::unique_ptr<GraphicsPipeline> skyPipeline;

    vk::Framebuffer framebuffer;

    void UpdatePipelines();

    void UpdateDescriptors() const;

    void DrawSky(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void DrawScene(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};
