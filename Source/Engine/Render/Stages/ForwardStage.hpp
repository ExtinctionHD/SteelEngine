#pragma once

#include "Engine/Render/Stages/RenderStage.hpp"
#include "Engine/Scene/Material.hpp"

class GraphicsPipeline;
class Scene;
class RenderPass;
class MaterialPipelineCache;

class ForwardStage : public RenderStage
{
public:
    ForwardStage(const SceneRenderContext& context_);

    ~ForwardStage() override;

    void RegisterScene(const Scene* scene_) override;

    void UpdateResources() override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) override;

    void Resize() override;

    void ReloadShaders() override;

private:
    std::unique_ptr<RenderPass> renderPass;
    std::unique_ptr<MaterialPipelineCache> pipelineCache;
    std::set<MaterialFlags> uniquePipelines;

    std::unique_ptr<GraphicsPipeline> skyboxPipeline;

    vk::Framebuffer framebuffer;

    void UpdatePipelines();

    void UpdateDescriptors() const;

    void DrawSkybox(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void DrawScene(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};
