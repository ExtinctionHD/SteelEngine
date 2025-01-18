#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

class Scene;
class RenderPass;
class MaterialPipelineCache;

class GBufferStage
{
public:
    GBufferStage();

    ~GBufferStage();

    void RegisterScene(const Scene* scene_);

    void RemoveScene();

    void Update();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void Resize();

    void ReloadShaders() const;

private:
    const Scene* scene = nullptr;

    std::unique_ptr<RenderPass> renderPass;
    std::unique_ptr<MaterialPipelineCache> pipelineCache;
    std::set<MaterialFlags> uniquePipelines;

    vk::Framebuffer framebuffer;

    void DrawScene(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};
