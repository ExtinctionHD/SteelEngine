#pragma once

#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Engine/Scene/GlobalIllumination.hpp"

class ScenePT;
class Scene;
class Camera;
class Environment;
class GBufferStage;
class LightingStage;
class ForwardStage;
struct KeyInput;

class HybridRenderer
{
public:
    HybridRenderer(const Scene* scene_, const ScenePT* scenePT_,
            const Camera* camera_, const Environment* environment_);

    ~HybridRenderer();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

private:
    const Scene* scene = nullptr;
    const ScenePT* scenePT = nullptr;
    const Camera* camera = nullptr;
    const Environment* environment = nullptr;

    LightVolume lightVolume;

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
