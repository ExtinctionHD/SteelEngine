#include "Engine/Render/SceneRenderer.hpp"

#include "Engine/Config.hpp"
#include "Engine/ConsoleVariable.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Components/EnvironmentComponent.hpp"
#include "Engine/Render/HybridRenderer.hpp"
#include "Engine/Render/PathTracingRenderer.hpp"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"

#include "Shaders/Common/Common.h"

namespace Details
{
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

    static RenderContextComponent CreateRenderContextComponent()
    {
        RenderContextComponent renderComponent;

        renderComponent.buffers.lights = ResourceContext::CreateBuffer({
            .type = BufferType::eUniform,
            .size = sizeof(gpu::Light) * MAX_LIGHT_COUNT,
            .usage = vk::BufferUsageFlagBits::eTransferDst,
            .stagingBuffer = true
        });

        renderComponent.buffers.materials = ResourceContext::CreateBuffer({
            .type = BufferType::eUniform,
            .size = sizeof(gpu::Light) * MAX_MATERIAL_COUNT,
            .usage = vk::BufferUsageFlagBits::eTransferDst,
            .stagingBuffer = true
        });

        renderComponent.buffers.frames.resize(VulkanContext::swapchain->GetImageCount());

        for (auto& frameBuffer : renderComponent.buffers.frames)
        {
            frameBuffer = ResourceContext::CreateBuffer({
                .type = BufferType::eUniform,
                .size = sizeof(gpu::Frame),
                .usage = vk::BufferUsageFlagBits::eTransferDst,
                .stagingBuffer = true
            });
        }

        return renderComponent;
    }

    static void UpdateLightBuffer(vk::CommandBuffer commandBuffer, const Scene& scene)
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
            const auto& renderComponent = scene.ctx().get<RenderContextComponent>();

            const BufferUpdate bufferUpdate{
                .data = GetByteView(lights),
                .blockedScope = SyncScope::kUniformRead
            };

            ResourceContext::UpdateBuffer(commandBuffer,
                    renderComponent.buffers.lights, bufferUpdate);
        }
    }

    static void UpdateMaterialBuffer(vk::CommandBuffer commandBuffer, const Scene& scene)
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
            const auto& renderComponent = scene.ctx().get<RenderContextComponent>();

            const BufferUpdate bufferUpdate{
                .data = GetByteView(materials),
                .blockedScope = SyncScope::kUniformRead
            };

            ResourceContext::UpdateBuffer(commandBuffer,
                    renderComponent.buffers.materials, bufferUpdate);
        }
    }

    static void UpdateFrameBuffer(vk::CommandBuffer commandBuffer, const Scene& scene, uint32_t imageIndex)
    {
        const auto& renderComponent = scene.ctx().get<RenderContextComponent>();
        const auto& cameraComponent = scene.ctx().get<CameraComponent>();

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
            {}
        };

        const BufferUpdate bufferUpdate{
            .data = GetByteView(frameData),
            .blockedScope = SyncScope::kUniformRead
        };

        const vk::Buffer frameBuffer = renderComponent.buffers.frames[imageIndex];

        ResourceContext::UpdateBuffer(commandBuffer, frameBuffer, bufferUpdate);
    }

    static void UpdateTlas(vk::CommandBuffer, Scene& scene)
    {
        TlasInstances tlasInstances;

        for (auto&& [entity, tc, rc] : scene.view<TransformComponent, RenderComponent>().each())
        {
            for (const auto& ro : rc.renderObjects)
            {
                tlasInstances.push_back(SceneHelpers::GetTlasInstance(scene, tc, ro));
            }
        }

        auto& renderComponent = scene.ctx().get<RenderContextComponent>();

        if (renderComponent.tlasInstanceCount != static_cast<uint32_t>(tlasInstances.size()))
        {
            if (renderComponent.tlas)
            {
                ResourceContext::DestroyResourceSafe(renderComponent.tlas);
            }

            if (!tlasInstances.empty())
            {
                const uint32_t instanceCount = static_cast<uint32_t>(tlasInstances.size());
                renderComponent.tlas = ResourceContext::CreateTlas(instanceCount);
                renderComponent.tlasInstanceCount = instanceCount;
            }
            else
            {
                renderComponent.tlas = nullptr;
                renderComponent.tlasInstanceCount = 0;
            }

            renderComponent.tlasUpdated = true;
        }

        if (!tlasInstances.empty())
        {
            // One time command is used to fix gpu crash caused by wrong synchronization
            VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
                {
                    // TODO move TLAS building to async compute queue
                    ResourceContext::BuildTlas(commandBuffer, renderComponent.tlas, tlasInstances);
                });
        }
    }
}

SceneRenderer::SceneRenderer()
{
    hybridRenderer = std::make_unique<HybridRenderer>();

    if (RenderOptions::pathTracingAllowed)
    {
        pathTracingRenderer = std::make_unique<PathTracingRenderer>();
    }

    renderComponent = Details::CreateRenderContextComponent();

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &SceneRenderer::HandleResizeEvent));

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &SceneRenderer::HandleKeyInputEvent));
}

SceneRenderer::~SceneRenderer()
{
    RemoveScene();

    if (renderComponent.tlas)
    {
        ResourceContext::DestroyResource(renderComponent.tlas);
    }

    if (renderComponent.buffers.lights)
    {
        ResourceContext::DestroyResource(renderComponent.buffers.lights);
    }

    ResourceContext::DestroyResource(renderComponent.buffers.materials);

    for (const auto frameBuffer : renderComponent.buffers.frames)
    {
        ResourceContext::DestroyResource(frameBuffer);
    }
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

    scene->ctx().emplace<RenderContextComponent&>(renderComponent);

    hybridRenderer->RegisterScene(scene);

    if (pathTracingRenderer)
    {
        pathTracingRenderer->RegisterScene(scene);
    }
}

void SceneRenderer::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    hybridRenderer->RemoveScene();

    if (pathTracingRenderer)
    {
        pathTracingRenderer->RemoveScene();
    }

    scene->ctx().erase<RenderContextComponent&>();

    scene = nullptr;
}

void SceneRenderer::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    if (scene)
    {
        Details::UpdateLightBuffer(commandBuffer, *scene);

        Details::UpdateFrameBuffer(commandBuffer, *scene, imageIndex);

        if (scene->ctx().get<MaterialStorageComponent>().updated)
        {
            Details::UpdateMaterialBuffer(commandBuffer, *scene);
        }

        if (RenderOptions::rayTracingAllowed)
        {
            Details::UpdateTlas(commandBuffer, *scene);
        }

        hybridRenderer->Update();

        if (pathTracingRenderer)
        {
            pathTracingRenderer->Update();
        }

        scene->ctx().get<TextureStorageComponent>().updated = false;
        scene->ctx().get<MaterialStorageComponent>().updated = false;
        scene->ctx().get<GeometryStorageComponent>().updated = false;

        renderComponent.tlasUpdated = false;
    }

    if (pathTracingRenderer && renderMode == RenderMode::ePathTracing)
    {
        pathTracingRenderer->Render(commandBuffer, imageIndex);
    }
    else
    {
        hybridRenderer->Render(commandBuffer, imageIndex);
    }
}

void SceneRenderer::HandleResizeEvent(const vk::Extent2D& extent) const
{
    VulkanContext::device->WaitIdle();

    if (extent.width != 0 && extent.height != 0)
    {
        hybridRenderer->Resize(extent);

        if (pathTracingRenderer)
        {
            pathTracingRenderer->Resize(extent);
        }
    }
}

void SceneRenderer::HandleKeyInputEvent(const KeyInput& keyInput)
{
    if (keyInput.action == KeyAction::ePress)
    {
        switch (keyInput.key)
        {
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
