#pragma once

#include "Vulkan/Resources/AccelerationStructureManager.hpp"

class Scene;
class HybridRenderer;
class PathTracingRenderer;
struct KeyInput;

enum class RenderMode
{
    eHybrid,
    ePathTracing
};

struct RenderSceneComponent
{
    vk::Buffer lightBuffer;
    vk::Buffer materialBuffer;
    std::vector<vk::Buffer> frameBuffers;

    uint32_t updateLightBuffer : 1;
    uint32_t updateMaterialBuffer : 1;
};

struct RayTracingSceneComponent
{
    TlasInstances tlasInstances;
    vk::AccelerationStructureKHR tlas;

    uint32_t buildTlas : 1;
};

class SceneRenderer
{
public:
    SceneRenderer();

    ~SceneRenderer();

    void RegisterScene(Scene* scene_);

    void RemoveScene();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

private:
    Scene* scene = nullptr;

    RenderMode renderMode = RenderMode::eHybrid;

    RenderSceneComponent renderSceneComponent;

    std::unique_ptr<HybridRenderer> hybridRenderer;
    std::unique_ptr<PathTracingRenderer> pathTracingRenderer;

    void HandleResizeEvent(const vk::Extent2D& extent) const;

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ToggleRenderMode();
};
