#pragma once

#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class Scene;
class GBufferStage;
class LightingStage;
class ForwardStage;
struct KeyInput;

class HybridRenderer
{
public:
    HybridRenderer(const Scene* scene_);

    ~HybridRenderer();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

private:
    const Scene* scene = nullptr;
    
    std::vector<Texture> gBufferTextures;

    std::unique_ptr<GBufferStage> gBufferStage;
    std::unique_ptr<LightingStage> lightingStage;
    std::unique_ptr<ForwardStage> forwardStage;

    void SetupGBufferTextures();

    void SetupRenderStages();

    void HandleResizeEvent(const vk::Extent2D& extent);

    void HandleKeyInputEvent(const KeyInput& keyInput) const;

    void ReloadShaders() const;
};
