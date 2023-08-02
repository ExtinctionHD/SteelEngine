#pragma once

class Scene;
class GBufferStage;
class LightingStage;
class ForwardStage;
struct KeyInput;

class HybridRenderer
{
public:
    HybridRenderer();

    ~HybridRenderer();

    void RegisterScene(const Scene* scene_);

    void UpdateScene() const;

    void RemoveScene();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void Resize(const vk::Extent2D& extent) const;

private:
    const Scene* scene = nullptr;

    std::unique_ptr<GBufferStage> gBufferStage;
    std::unique_ptr<LightingStage> lightingStage;
    std::unique_ptr<ForwardStage> forwardStage;

    void HandleKeyInputEvent(const KeyInput& keyInput) const;

    void ReloadShaders() const;
};
