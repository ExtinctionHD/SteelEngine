#pragma once

#include "Engine/Scene/Components/Components.hpp"
#include "Vulkan/Resources/ImageHelpers.hpp"
#include "Vulkan/Resources/TextureHelpers.hpp"

class Scene;
class HybridRenderer;
class PathTracingRenderer;
struct KeyInput;

enum class RenderMode
{
    eHybrid,
    ePathTracing
};

struct AtmosphereLUTs
{
    Texture transmittance;
    Texture multiScattering;
    Texture arial;
    Texture sky;
};

struct LightingProbe
{
    Texture irradiance;
    Texture reflection;
    Texture specularLUT;
};

struct GBufferAttachments
{
    RenderTarget depthStencil;
    RenderTarget sceneColor;
    RenderTarget normals;
    RenderTarget baseColorOcclusion;
    RenderTarget roughnessMetallic;
};

struct UniformBuffers
{
    vk::Buffer lights;
    vk::Buffer materials;
    std::vector<vk::Buffer> frames;
};

struct RenderContextComponent
{
    AtmosphereLUTs atmosphereLUTs;
    LightingProbe lightingProbe;
    GBufferAttachments gBuffer;
    UniformBuffers buffers;

    vk::AccelerationStructureKHR tlas;
    uint32_t tlasInstanceCount = 0;
    bool tlasUpdated = false;
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

    RenderContextComponent renderComponent;

    std::unique_ptr<HybridRenderer> hybridRenderer;
    std::unique_ptr<PathTracingRenderer> pathTracingRenderer;

    void HandleResizeEvent(const vk::Extent2D& extent) const;

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ToggleRenderMode();
};
