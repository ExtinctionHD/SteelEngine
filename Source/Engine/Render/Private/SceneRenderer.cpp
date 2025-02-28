#include "Engine/Render/SceneRenderer.hpp"

#include "Engine/Config.hpp"
#include "Engine/ConsoleVariable.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Components/EnvironmentComponent.hpp"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/Stages/AtmosphereStage.hpp"
#include "Engine/Render/Stages/DebugDrawStage.hpp"
#include "Engine/Render/Stages/PathTracingStage.hpp"
#include "Engine/Render/Stages/TranslucentStage.hpp"
#include "Engine/Render/Stages/DeferredStage.hpp"
#include "Engine/Render/Stages/LightingStage.hpp"
#include "Engine/Render/Stages/PostProcessStage.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"

#include "Shaders/Common/Common.h"

namespace Details
{
    static int32_t specularLutExtent = 512;
    static CVarInt specularLutExtentCVar(
            "r.probe.specularExtent", specularLutExtent);

    static int32_t irradianceProbeExtent = 128;
    static CVarInt irradianceProbeExtentCVar(
            "r.probe.irradianceExtent", irradianceProbeExtent);

    static int32_t reflectionProbeExtent = 256;
    static CVarInt reflectionProbeExtentCVar(
            "r.probe.reflectionExtent", reflectionProbeExtent);

    static bool debugDrawEnabled = false;
    static CVarBool debugDrawEnabledCVar(
            "r.DebugDraw.Enabled", debugDrawEnabled);

    static void EmplaceDefaultCamera(Scene& scene)
    {
        const entt::entity entity = scene.CreateEntity(entt::null, {});

        auto& cc = scene.emplace<CameraComponent>(entity);

        cc.location = Config::DefaultCamera::kLocation;
        cc.projection = Config::DefaultCamera::kProjection;

        cc.viewMatrix = CameraHelpers::ComputeViewMatrix(cc.location);
        cc.projMatrix = CameraHelpers::ComputeProjMatrix(cc.projection);

        scene.ctx().emplace<CameraComponent&>(cc);
    }

    static void EmplaceDefaultEnvironment(Scene& scene)
    {
        static const CVarString& envDefaultPathCVar = CVarString::Get("scene.EnvDefaultPath");

        const entt::entity entity = scene.CreateEntity(entt::null, {});

        auto& ec = scene.emplace<EnvironmentComponent>(entity);

        ec = EnvironmentHelpers::LoadEnvironment(Filepath(envDefaultPathCVar.GetValue()));

        scene.ctx().emplace<EnvironmentComponent&>(ec);
    }

    static void EmplaceDefaultAtmosphere(Scene& scene)
    {
        const entt::entity entity = scene.CreateEntity(entt::null, {});

        auto& ac = scene.emplace<AtmosphereComponent>(entity);

        scene.ctx().emplace<AtmosphereComponent&>(ac);
    }

    static AtmosphereLUTs CreateAtmosphereLUTs()
    {
        AtmosphereLUTs atmosphereLUTs;

        // TODO check sampler parameters
        constexpr SamplerDescription kSamplerDescription{
            .magFilter = vk::Filter::eNearest,
            .minFilter = vk::Filter::eNearest,
            .mipmapMode = vk::SamplerMipmapMode::eNearest,
            .addressMode = vk::SamplerAddressMode::eClampToEdge,
            .maxAnisotropy = 0.0f,
            .minLod = 0.0f,
            .maxLod = 0.0f,
        };

        // TODO select optimal formats

        atmosphereLUTs.transmittance.sampler = TextureCache::GetSampler(kSamplerDescription);
        atmosphereLUTs.transmittance.image = ResourceContext::CreateBaseImage({
            .format = vk::Format::eR32G32B32A32Sfloat,
            .extent = VulkanHelpers::GetExtent(RenderOptions::Atmosphere::transmittanceLutExtent),
            .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled
        });

        atmosphereLUTs.multiScattering.sampler = TextureCache::GetSampler(kSamplerDescription);
        atmosphereLUTs.multiScattering.image = ResourceContext::CreateBaseImage({
            .format = vk::Format::eR32G32B32A32Sfloat,
            .extent = VulkanHelpers::GetExtent(RenderOptions::Atmosphere::multiScatteringLutExtent),
            .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled
        });

        atmosphereLUTs.arial.sampler = TextureCache::GetSampler(kSamplerDescription);
        atmosphereLUTs.arial.image = ResourceContext::CreateBaseImage({
            .type = ImageType::e3D,
            .format = vk::Format::eR32G32B32A32Sfloat,
            .extent = VulkanHelpers::GetExtent(RenderOptions::Atmosphere::arialLutExtent),
            .depth = static_cast<uint32_t>(RenderOptions::Atmosphere::arialLutDepth),
            .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled
        });

        atmosphereLUTs.sky.sampler = TextureCache::GetSampler(kSamplerDescription);
        atmosphereLUTs.sky.image = ResourceContext::CreateBaseImage({
            .format = vk::Format::eR32G32B32A32Sfloat,
            .extent = VulkanHelpers::GetExtent(RenderOptions::Atmosphere::skyLutExtent),
            .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled
        });

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    PipelineBarrier::kEmpty
                };

                for (const auto& [image, sampler] : atmosphereLUTs.GetArray())
                {
                    ImageHelpers::TransitImageLayout(commandBuffer, image.image,
                            ImageHelpers::kFlatColor, layoutTransition);
                }
            });

        return atmosphereLUTs;
    }

    static LightingProbe CreateLightingProbe()
    {
        LightingProbe lightingProbe;

        constexpr SamplerDescription kIrradianceSamplerDescription{
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
            .mipmapMode = vk::SamplerMipmapMode::eNearest,
            .addressMode = vk::SamplerAddressMode::eRepeat,
            .maxAnisotropy = 0.0f,
            .minLod = 0.0f,
            .maxLod = 0.0f,
        };

        constexpr SamplerDescription kReflectionSamplerDescription{
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
            .mipmapMode = vk::SamplerMipmapMode::eLinear,
            .addressMode = vk::SamplerAddressMode::eRepeat,
            .maxAnisotropy = 0.0f,
        };

        constexpr SamplerDescription kSpecularLutSamplerDescription{
            .magFilter = vk::Filter::eNearest,
            .minFilter = vk::Filter::eNearest,
            .mipmapMode = vk::SamplerMipmapMode::eNearest,
            .addressMode = vk::SamplerAddressMode::eClampToEdge,
            .maxAnisotropy = 0.0f,
            .minLod = 0.0f,
            .maxLod = 0.0f,
        };

        lightingProbe.irradiance.sampler = TextureCache::GetSampler(kIrradianceSamplerDescription);
        lightingProbe.irradiance.image = ResourceContext::CreateCubeImage({
            .format = vk::Format::eB10G11R11UfloatPack32,
            .extent = VulkanHelpers::GetExtent(irradianceProbeExtent),
            .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled
        });


        lightingProbe.reflection.sampler = TextureCache::GetSampler(kReflectionSamplerDescription);
        lightingProbe.reflection.image = ResourceContext::CreateCubeImage({
            .format = vk::Format::eB10G11R11UfloatPack32,
            .extent = VulkanHelpers::GetExtent(reflectionProbeExtent),
            .mipLevelCount = ImageHelpers::CalculateMipLevelCount(VulkanHelpers::GetExtent(reflectionProbeExtent)),
            .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled
        });

        lightingProbe.specularLUT.sampler = TextureCache::GetSampler(kSpecularLutSamplerDescription);
        lightingProbe.specularLUT.image = ResourceContext::CreateBaseImage({
            .format = vk::Format::eR16G16Sfloat,
            .extent = VulkanHelpers::GetExtent(specularLutExtent),
            .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled
        });

        return lightingProbe;
    }

    static GBufferAttachments CreateGBuffer(vk::Extent2D extent)
    {
        GBufferAttachments gBuffer;

        for (size_t i = 0; i < GBufferAttachments::GetCount(); ++i)
        {
            constexpr vk::ImageUsageFlags colorImageUsage
                    = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage;

            constexpr vk::ImageUsageFlags depthImageUsage
                    = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;

            const bool isDepth = ImageHelpers::IsDepthFormat(GBufferFormats::kFormats[i]);

            const vk::ImageUsageFlags imageUsage = isDepth ? depthImageUsage : colorImageUsage;

            gBuffer.AccessArray()[i] = ResourceContext::CreateBaseImage({
                .format = GBufferFormats::kFormats[i],
                .extent = extent,
                .sampleCount = vk::SampleCountFlagBits::e1,
                .usage = imageUsage
            });
        }

        VulkanContext::device->ExecuteOneTimeCommands([&gBuffer](vk::CommandBuffer commandBuffer)
            {
                const ImageLayoutTransition colorLayoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eGeneral,
                    PipelineBarrier::kEmpty
                };

                const ImageLayoutTransition depthLayoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    PipelineBarrier::kEmpty
                };

                for (size_t i = 0; i < GBufferAttachments::GetCount(); ++i)
                {
                    const vk::Image image = gBuffer.GetArray()[i].image;

                    const bool isDepth = ImageHelpers::IsDepthFormat(GBufferFormats::kFormats[i]);

                    const vk::ImageSubresourceRange& subresourceRange = isDepth
                            ? ImageHelpers::kFlatDepth : ImageHelpers::kFlatColor;

                    const ImageLayoutTransition& layoutTransition = isDepth
                            ? depthLayoutTransition : colorLayoutTransition;

                    ImageHelpers::TransitImageLayout(commandBuffer, image, subresourceRange, layoutTransition);
                }
            });

        return gBuffer;
    }

    static GlobalUniforms CreateUniforms(uint32_t frameCount)
    {
        GlobalUniforms uniforms;

        uniforms.lights = ResourceContext::CreateBuffer({
            .type = BufferType::eUniform,
            .size = sizeof(gpu::Light) * MAX_LIGHT_COUNT,
            .usage = vk::BufferUsageFlagBits::eTransferDst,
            .stagingBuffer = true
        });

        uniforms.materials = ResourceContext::CreateBuffer({
            .type = BufferType::eUniform,
            .size = sizeof(gpu::Light) * MAX_MATERIAL_COUNT,
            .usage = vk::BufferUsageFlagBits::eTransferDst,
            .stagingBuffer = true
        });

        uniforms.frames.resize(frameCount);

        for (auto& frameBuffer : uniforms.frames)
        {
            frameBuffer = ResourceContext::CreateBuffer({
                .type = BufferType::eUniform,
                .size = sizeof(gpu::Frame),
                .usage = vk::BufferUsageFlagBits::eTransferDst,
                .stagingBuffer = true
            });
        }

        return uniforms;
    }

    static void DestroyGBuffer(const GBufferAttachments& gBuffer)
    {
        for (const RenderTarget& target : gBuffer.GetArray())
        {
            ResourceContext::DestroyResource(target);
        }
    }

    static void DestroyUniforms(const GlobalUniforms& uniforms)
    {
        if (uniforms.lights)
        {
            ResourceContext::DestroyResource(uniforms.lights);
        }

        ResourceContext::DestroyResource(uniforms.materials);

        for (const vk::Buffer buffer : uniforms.frames)
        {
            ResourceContext::DestroyResource(buffer);
        }
    }

    static void UpdateLightBuffer(vk::CommandBuffer commandBuffer,
            const Scene& scene, const GlobalUniforms& uniforms)
    {
        const auto sceneLightsView = scene.view<TransformComponent, LightComponent>();

        std::vector<gpu::Light> lights;
        lights.reserve(sceneLightsView.size_hint());

        for (auto&& [entity, tc, lc] : sceneLightsView.each())
        {
            gpu::Light light{};

            if (lc.type == LightType::eDirectional)
            {
                const glm::vec3 direction = tc.GetWorldTransform().GetAxis(Axis::eX);

                light.location = glm::vec4(-direction, 0.0f);
            }
            else if (lc.type == LightType::ePoint)
            {
                const glm::vec3 position = tc.GetWorldTransform().GetTranslation();

                light.location = glm::vec4(position, 1.0f);
            }

            light.color = lc.color;

            lights.push_back(light);
        }

        if (!lights.empty())
        {
            const BufferUpdate bufferUpdate{
                .data = GetByteView(lights),
                .blockedScope = SyncScope::kUniformRead
            };

            ResourceContext::UpdateBuffer(commandBuffer, uniforms.lights, bufferUpdate);
        }
    }

    static void UpdateMaterialBuffer(vk::CommandBuffer commandBuffer,
            const Scene& scene, const GlobalUniforms& uniforms)
    {
        const auto& materialComponent = scene.ctx().get<MaterialStorageComponent>();

        std::vector<gpu::Material> materials;
        materials.reserve(materialComponent.materials.size());

        for (const Material& material : materialComponent.materials)
        {
            materials.push_back(material.data);
        }

        if (!materials.empty())
        {
            const BufferUpdate bufferUpdate{
                .data = GetByteView(materials),
                .blockedScope = SyncScope::kUniformRead
            };

            ResourceContext::UpdateBuffer(commandBuffer, uniforms.materials, bufferUpdate);
        }
    }

    static void UpdateFrameBuffer(vk::CommandBuffer commandBuffer,
            const Scene& scene, const GlobalUniforms& uniforms, uint32_t frameIndex)
    {
        const auto& cameraComponent = scene.ctx().get<CameraComponent>();
        const auto& atmosphereComponent = scene.ctx().get<AtmosphereComponent>();

        const glm::mat4 viewProjMatrix = cameraComponent.projMatrix * cameraComponent.viewMatrix;

        const glm::mat4 inverseViewMatrix = glm::inverse(cameraComponent.viewMatrix);
        const glm::mat4 inverseProjMatrix = glm::inverse(cameraComponent.projMatrix);

        const gpu::Frame frameData{
            cameraComponent.viewMatrix,
            cameraComponent.projMatrix,
            viewProjMatrix,
            inverseViewMatrix,
            inverseProjMatrix,
            inverseViewMatrix * inverseProjMatrix,
            cameraComponent.location.position,
            cameraComponent.projection.zNear,
            cameraComponent.projection.zFar,
            Timer::GetGlobalSeconds(),
            atmosphereComponent,
        };

        const BufferUpdate bufferUpdate{
            .data = GetByteView(frameData),
            .blockedScope = SyncScope::kUniformRead
        };

        ResourceContext::UpdateBuffer(commandBuffer, uniforms.frames[frameIndex], bufferUpdate);
    }

    static void UpdateTlas(vk::CommandBuffer, const Scene& scene, TopLevelAS& tlas)
    {
        TlasInstances tlasInstances;

        for (auto&& [entity, tc, rc] : scene.view<TransformComponent, RenderComponent>().each())
        {
            for (const auto& ro : rc.renderObjects)
            {
                tlasInstances.push_back(SceneHelpers::GetTlasInstance(scene, tc, ro));
            }
        }

        if (tlas.instanceCount != static_cast<uint32_t>(tlasInstances.size()))
        {
            if (tlas)
            {
                ResourceContext::DestroyResourceSafe(tlas);
            }

            if (!tlasInstances.empty())
            {
                const auto instanceCount = static_cast<uint32_t>(tlasInstances.size());
                tlas = ResourceContext::CreateTlas(instanceCount);
                tlas.instanceCount = instanceCount;
            }
            else
            {
                tlas = nullptr;
                tlas.instanceCount = 0;
            }

            tlas.updated = true;
        }

        if (!tlasInstances.empty())
        {
            // One time command is used to fix gpu crash caused by wrong synchronization
            VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
                {
                    // TODO move TLAS building to async compute queue
                    ResourceContext::BuildTlas(commandBuffer, tlas, tlasInstances);
                });
        }
    }
}

vk::Extent2D GBufferAttachments::GetExtent() const
{
    if (sceneColor.image)
    {
        return ResourceContext::GetImageDescription(sceneColor.image).extent;
    }

    return vk::Extent2D();
}

uint32_t GlobalUniforms::GetFrameCount() const
{
    return static_cast<uint32_t>(frames.size());
}

vk::Extent2D SceneRenderContext::GetRenderExtent() const
{
    return gBuffer.GetExtent();
}

SceneRenderer::SceneRenderer()
{
    context.atmosphereLUTs = Details::CreateAtmosphereLUTs();
    context.lightingProbe = Details::CreateLightingProbe();

    context.gBuffer = Details::CreateGBuffer(VulkanContext::swapchain->GetExtent());
    context.uniforms = Details::CreateUniforms(VulkanContext::swapchain->GetImageCount());

    stages.atmosphere = std::make_unique<AtmosphereStage>(context);
    stages.deferred = std::make_unique<DeferredStage>(context);
    stages.lighting = std::make_unique<LightingStage>(context);
    stages.translucent = std::make_unique<TranslucentStage>(context);
    stages.postProcess = std::make_unique<PostProcessStage>(context);
    stages.debugDraw = std::make_unique<DebugDrawStage>(context);

    if (RenderOptions::pathTracingAllowed)
    {
        stages.pathTracing = std::make_unique<PathTracingStage>(context);
    }

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &SceneRenderer::HandleResizeEvent));

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &SceneRenderer::HandleKeyInputEvent));
}

SceneRenderer::~SceneRenderer()
{
    RemoveScene();

    if (context.tlas)
    {
        ResourceContext::DestroyResource(context.tlas);
    }

    for (const Texture& texture : context.atmosphereLUTs.GetArray())
    {
        ResourceContext::DestroyResource(texture.image);
    }

    for (const Texture& texture : context.lightingProbe.GetArray())
    {
        ResourceContext::DestroyResource(texture.image);
    }

    Details::DestroyGBuffer(context.gBuffer);
    Details::DestroyUniforms(context.uniforms);
}

void SceneRenderer::RegisterScene(Scene* scene_)
{
    EASY_FUNCTION()

    RemoveScene();

    scene = scene_;
    Assert(scene);

    if (!scene->ctx().contains<CameraComponent&>())
    {
        Details::EmplaceDefaultCamera(*scene);
    }

    if (!scene->ctx().contains<EnvironmentComponent&>())
    {
        Details::EmplaceDefaultEnvironment(*scene);
    }

    if (!scene->ctx().contains<AtmosphereComponent&>())
    {
        Details::EmplaceDefaultAtmosphere(*scene);
    }

    stages.ForEach(&RenderStage::RegisterScene, scene);
}

void SceneRenderer::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    stages.ForEach(&RenderStage::RemoveScene);

    scene = nullptr;
}

void SceneRenderer::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    if (scene)
    {
        Details::UpdateLightBuffer(commandBuffer, *scene, context.uniforms);

        Details::UpdateFrameBuffer(commandBuffer, *scene, context.uniforms, imageIndex);

        // TODO make material buffer update each frame
        if (scene->ctx().get<MaterialStorageComponent>().updated)
        {
            Details::UpdateMaterialBuffer(commandBuffer, *scene, context.uniforms);
        }

        if (RenderOptions::rayTracingAllowed)
        {
            Details::UpdateTlas(commandBuffer, *scene, context.tlas);
        }

        stages.ForEach(&RenderStage::Update);

        scene->ctx().get<TextureStorageComponent>().updated = false;
        scene->ctx().get<MaterialStorageComponent>().updated = false;
        scene->ctx().get<GeometryStorageComponent>().updated = false;

        context.tlas.updated = false;
    }

    stages.atmosphere->Render(commandBuffer, imageIndex);

    // TODO handle null scene correctly
    if (stages.pathTracing && renderMode == RenderMode::ePathTracing)
    {
        stages.pathTracing->Render(commandBuffer, imageIndex);
    }
    else
    {
        stages.deferred->Render(commandBuffer, imageIndex);
        stages.lighting->Render(commandBuffer, imageIndex);
        stages.translucent->Render(commandBuffer, imageIndex);
    }

    stages.postProcess->Render(commandBuffer, imageIndex);

    if (Details::debugDrawEnabled)
    {
        stages.debugDraw->Render(commandBuffer, imageIndex);
    }
}

void SceneRenderer::HandleResizeEvent(const vk::Extent2D& extent)
{
    VulkanContext::device->WaitIdle();

    if (extent.width > 0 && extent.height > 0)
    {
        Details::DestroyGBuffer(context.gBuffer);

        context.gBuffer = Details::CreateGBuffer(VulkanContext::swapchain->GetExtent());

        Assert(VulkanContext::swapchain->GetImageCount() == context.uniforms.GetFrameCount());

        stages.ForEach(&RenderStage::Resize);
    }
}

void SceneRenderer::HandleKeyInputEvent(const KeyInput& keyInput)
{
    if (keyInput.action == KeyAction::ePress)
    {
        switch (keyInput.key)
        {
        case Key::eR:
            ReloadShaders();
            break;
        case Key::eT:
            ToggleRenderMode();
            break;
        default:
            break;
        }
    }
}

void SceneRenderer::ToggleRenderMode()
{
    uint32_t i = static_cast<const uint32_t>(renderMode);

    i = (i + 1) % kRenderModeCount;

    renderMode = static_cast<RenderMode>(i);
}

void SceneRenderer::ReloadShaders() const
{
    VulkanContext::device->WaitIdle();

    stages.ForEach(&RenderStage::ReloadShaders);
}
