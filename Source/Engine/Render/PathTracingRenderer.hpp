#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Vulkan/Resources/DescriptorProvider.hpp"
#include "Vulkan/VulkanHelpers.hpp"

class Scene;
class RayTracingPipeline;
struct KeyInput;

class PathTracingRenderer
{
public:
    PathTracingRenderer();

    virtual ~PathTracingRenderer();

    void RegisterScene(const Scene* scene_);

    void RemoveScene();

    void Update() const;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

    void Resize(const vk::Extent2D& extent);

private:
    const Scene* scene = nullptr;

    Texture accumulationTexture;

    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;

    std::unique_ptr<DescriptorProvider> descriptorProvider;

    uint32_t accumulationIndex = 0;

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ReloadShaders();

    void ResetAccumulation();
};
