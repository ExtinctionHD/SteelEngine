#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Vulkan/Resources/DescriptorProvider.hpp"
#include "Vulkan/Resources/ImageHelpers.hpp"
#include "Vulkan/VulkanHelpers.hpp"

class Scene;
class RayTracingPipeline;
struct KeyInput;

class PathTracingStage
{
public:
    PathTracingStage();

    virtual ~PathTracingStage();

    void RegisterScene(const Scene* scene_);

    void RemoveScene();

    void Update() const;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

    void Resize();

private:
    const Scene* scene = nullptr;

    RenderTarget accumulationTarget;

    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;
    std::unique_ptr<DescriptorProvider> descriptorProvider;

    uint32_t accumulationIndex = 0;

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ReloadShaders();

    void ResetAccumulation();
};
