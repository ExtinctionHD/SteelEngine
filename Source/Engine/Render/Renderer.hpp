#pragma once

#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Engine/Scene/GlobalIllumination.hpp"

class Scene;
class Camera;
class Environment;
class GBufferStage;
class LightingStage;
class ForwardStage;
struct KeyInput;

class Renderer
{
public:
    Renderer(Scene* scene_, Camera* camera_, Environment* environment_);
    ~Renderer();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

private:
    Scene* scene = nullptr;
    Camera* camera = nullptr;
    Environment* environment = nullptr;

    SphericalHarmonicsGrid sphericalHarmonicsGrid;

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
