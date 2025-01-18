#pragma once

#include "Engine/Scene/Components/Components.hpp"
#include "Vulkan/Resources/ImageHelpers.hpp"
#include "Vulkan/Resources/TextureHelpers.hpp"

#include "Utils/Helpers.hpp"


class PostProcessStage;
class LightingStage;
class ForwardStage;
class GBufferStage;
class Scene;
class PathTracingStage;
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

    DEFINE_ARRAY_FUNCTIONS(AtmosphereLUTs, Texture)
};

struct LightingProbe
{
    Texture irradiance;
    Texture reflection;
    Texture specularLUT;

    DEFINE_ARRAY_FUNCTIONS(LightingProbe, Texture)
};

struct GBufferAttachments
{
    RenderTarget normal;
    RenderTarget sceneColor;
    RenderTarget baseColorOcclusion;
    RenderTarget roughnessMetallic;
    RenderTarget depthStencil;

    vk::Extent2D GetExtent() const;

    DEFINE_ARRAY_FUNCTIONS(GBufferAttachments, RenderTarget)
};

struct GBufferFormats
{
    static constexpr vk::Format kNormal = vk::Format::eA2B10G10R10UnormPack32;
    static constexpr vk::Format kSceneColor = vk::Format::eB10G11R11UfloatPack32;
    static constexpr vk::Format kBaseColorOcclusion = vk::Format::eR8G8B8A8Unorm;
    static constexpr vk::Format kRoughnessMetallic = vk::Format::eR8G8Unorm;
    static constexpr vk::Format kDepthStencil = vk::Format::eD32Sfloat;

    static constexpr uint32_t kColorCount = GBufferAttachments::GetCount() - 1;

    static constexpr std::array<vk::Format, GBufferAttachments::GetCount()> kFormats{
        kNormal,
        kSceneColor,
        kBaseColorOcclusion,
        kRoughnessMetallic,
        kDepthStencil
    };
};

struct GlobalUniforms
{
    vk::Buffer lights;
    vk::Buffer materials;
    std::vector<vk::Buffer> frames;
};

struct TopLevelAS : vk::AccelerationStructureKHR
{
    uint32_t instanceCount : 31 = 0;
    uint32_t updated : 1 = false;

    TopLevelAS& operator=(vk::AccelerationStructureKHR as)
    {
        *this = TopLevelAS(as);
        return *this;
    }
};

// TODO make not a component
struct RenderContextComponent
{
    AtmosphereLUTs atmosphereLUTs;
    LightingProbe lightingProbe;
    GBufferAttachments gBuffer;
    GlobalUniforms uniforms;
    TopLevelAS tlas;

    // TODO add GetRenderExtent
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

    std::unique_ptr<GBufferStage> gBufferStage;
    std::unique_ptr<LightingStage> lightingStage;
    std::unique_ptr<ForwardStage> forwardStage;
    std::unique_ptr<PostProcessStage> postProcessStage;
    std::unique_ptr<PathTracingStage> pathTracingStage;

    void HandleResizeEvent(const vk::Extent2D& extent);

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ToggleRenderMode();

    void ReloadShaders() const;
};
