#pragma once

class Scene;
class HybridRenderer;
class PathTracingRenderer;
struct KeyInput;

enum class RenderMode
{
    eHybrid,
    ePathTracing
};

class SceneRenderer
{
public:
    SceneRenderer();

    ~SceneRenderer();

    void RegisterScene(const Scene* scene_);

    void RemoveScene();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

private:
    const Scene* scene = nullptr;

    RenderMode renderMode = RenderMode::eHybrid;

    std::unique_ptr<HybridRenderer> hybridRenderer;
    std::unique_ptr<PathTracingRenderer> pathTracingRenderer;

    void HandleResizeEvent(const vk::Extent2D& extent) const;

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ToggleRenderMode();
};
