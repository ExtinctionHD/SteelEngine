#pragma once

class Scene;
struct SceneRenderContext;

class RenderStage
{
public:
    RenderStage(const SceneRenderContext& context_);

    virtual ~RenderStage() = default;

    virtual void RegisterScene(const Scene* scene_);

    virtual void RemoveScene();

    virtual void UpdateResources() {}

    virtual void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) = 0;

    virtual void Resize() {}

    virtual void ReloadShaders() = 0;

protected:
    const SceneRenderContext& context;
    const Scene* scene = nullptr;
};
