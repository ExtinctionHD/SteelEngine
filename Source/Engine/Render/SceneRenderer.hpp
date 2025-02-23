#pragma once

#include "Engine/Scene/Components/Components.hpp"
#include "Vulkan/Resources/ImageHelpers.hpp"
#include "Vulkan/Resources/TextureHelpers.hpp"

#include "Utils/Helpers.hpp"


class PathTracingStage;
class PostProcessStage;
class LightingStage;
class TranslucentStage;
class DeferredStage;
class RenderStage;
class Scene;
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

// TODO use per frame copies for all uniforms
struct GlobalUniforms
{
    vk::Buffer lights;
    vk::Buffer materials;
    std::vector<vk::Buffer> frames;

    uint32_t GetFrameCount() const;
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

// TODO rename RenderContext after removing global one
struct SceneRenderContext
{
    AtmosphereLUTs atmosphereLUTs;
    LightingProbe lightingProbe;
    GBufferAttachments gBuffer;
    GlobalUniforms uniforms;
    TopLevelAS tlas;

    vk::Extent2D GetRenderExtent() const;
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
    struct RenderStages
    {
        std::unique_ptr<DeferredStage> deferred;
        std::unique_ptr<LightingStage> lighting;
        std::unique_ptr<TranslucentStage> translucent;
        std::unique_ptr<PostProcessStage> postProcess;
        std::unique_ptr<PathTracingStage> pathTracing;

        DEFINE_ARRAY_FUNCTIONS(RenderStages, std::unique_ptr<RenderStage>)

        template <typename Func, typename... Args>
        void ForEach(Func&& method, Args&&... args) const
        {
            for (const auto& stage : GetArray())
            {
                if (stage)
                {
                    std::invoke(std::forward<Func>(method), stage.get(), std::forward<Args>(args)...);
                }
            }
        }
    };

    SceneRenderContext context;

    Scene* scene = nullptr;

    RenderStages stages;

    RenderMode renderMode = RenderMode::eHybrid; // TODO convert into cvar

    void HandleResizeEvent(const vk::Extent2D& extent);

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ToggleRenderMode();

    void ReloadShaders() const;
};
