#pragma once

#include "Engine/Render/Stages/RenderStage.hpp"
#include "Engine/Scene/Material.hpp"

class Scene;
class RenderPass;
class MaterialPipelineCache;

class DeferredStage : public RenderStage
{
public:
    DeferredStage(const SceneRenderContext& context_);

    ~DeferredStage() override;

    void RegisterScene(const Scene* scene_) override;

    void RemoveScene() override;

    void Update() override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const override;

    void Resize() override;

    void ReloadShaders() override;

private:
    std::unique_ptr<RenderPass> renderPass;
    std::unique_ptr<MaterialPipelineCache> pipelineCache;
    std::set<MaterialFlags> uniquePipelines;

    vk::Framebuffer framebuffer;

    void DrawScene(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};
