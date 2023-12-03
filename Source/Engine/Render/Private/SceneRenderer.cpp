#include "Engine/Render/SceneRenderer.hpp"

#include "Engine/Config.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Components/EnvironmentComponent.hpp"
#include "Engine/Render/HybridRenderer.hpp"
#include "Engine/Render/PathTracingRenderer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"

#include "Shaders/Common/Common.h"

namespace Details
{
    static void EmplaceDefaultCamera(Scene& scene)
    {
        const entt::entity entity = scene.create();

        auto& cc = scene.emplace<CameraComponent>(entity);

        cc.location = Config::DefaultCamera::kLocation;
        cc.projection = Config::DefaultCamera::kProjection;

        cc.viewMatrix = CameraHelpers::ComputeViewMatrix(cc.location);
        cc.projMatrix = CameraHelpers::ComputeProjMatrix(cc.projection);

        scene.ctx().emplace<CameraComponent&>(cc);
    }

    static void EmplaceDefaultEnvironment(Scene& scene)
    {
        const entt::entity entity = scene.create();

        auto& ec = scene.emplace<EnvironmentComponent>(entity);

        ec = EnvironmentHelpers::LoadEnvironment(Config::kDefaultPanoramaPath);

        scene.ctx().emplace<EnvironmentComponent>(ec);
    }

    static RenderContextComponent CreateRenderContextComponent()
    {
        RenderContextComponent renderComponent;

        renderComponent.lightBuffer = ResourceContext::CreateBuffer({
            .type = BufferType::eUniform,
            .size = sizeof(gpu::Light) * MAX_LIGHT_COUNT,
            .usage = vk::BufferUsageFlagBits::eTransferDst,
            .stagingBuffer = true
        });

        renderComponent.materialBuffer = ResourceContext::CreateBuffer({
            .type = BufferType::eUniform,
            .size = sizeof(gpu::Light) * MAX_MATERIAL_COUNT,
            .usage = vk::BufferUsageFlagBits::eTransferDst,
            .stagingBuffer = true
        });

        renderComponent.frameBuffers.resize(VulkanContext::swapchain->GetImageCount());

        for (auto& frameBuffer : renderComponent.frameBuffers)
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
                    renderComponent.lightBuffer, bufferUpdate);
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
                    renderComponent.materialBuffer, bufferUpdate);
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

        ResourceContext::UpdateBuffer(commandBuffer,
                renderComponent.frameBuffers[imageIndex], bufferUpdate);
    }

    static void UpdateTlas(vk::CommandBuffer commandBuffer, Scene& scene)
    {
        TlasInstances tlasInstances;

        for (auto&& [entity, tc, rc] : scene.view<TransformComponent, RenderComponent>().each())
        {
            for (const auto& ro : rc.renderObjects)
            {
                tlasInstances.push_back(SceneHelpers::GetTlasInstance(scene, tc, ro));
            }
        }

        auto& rayTracingComponent = scene.ctx().get<RayTracingContextComponent>();

        if (rayTracingComponent.tlasInstanceCount != static_cast<uint32_t>(tlasInstances.size()))
        {
            if (rayTracingComponent.tlas)
            {
                ResourceContext::DestroyResourceSafe(rayTracingComponent.tlas);
            }

            if (!tlasInstances.empty())
            {
                rayTracingComponent.tlas = ResourceContext::CreateTlas(tlasInstances);
                rayTracingComponent.tlasInstanceCount = static_cast<uint32_t>(tlasInstances.size());
                rayTracingComponent.updated = true;
            }
            else
            {
                rayTracingComponent.tlas = nullptr;
                rayTracingComponent.tlasInstanceCount = 0;
                rayTracingComponent.updated = true;
            }
        }

        if (!tlasInstances.empty())
        {
            ResourceContext::BuildTlas(commandBuffer, rayTracingComponent.tlas, tlasInstances);
        }
    }
}

SceneRenderer::SceneRenderer()
{
    hybridRenderer = std::make_unique<HybridRenderer>();

    if constexpr (Config::kPathTracingEnabled)
    {
        pathTracingRenderer = std::make_unique<PathTracingRenderer>();
    }

    renderComponent = Details::CreateRenderContextComponent();

    rayTracingComponent = RayTracingContextComponent{};

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &SceneRenderer::HandleResizeEvent));

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &SceneRenderer::HandleKeyInputEvent));
}

SceneRenderer::~SceneRenderer()
{
    RemoveScene();

    if (rayTracingComponent.tlas)
    {
        ResourceContext::DestroyResource(rayTracingComponent.tlas);
    }

    if (renderComponent.lightBuffer)
    {
        ResourceContext::DestroyResource(renderComponent.lightBuffer);
    }

    ResourceContext::DestroyResource(renderComponent.materialBuffer);

    for (const auto frameBuffer : renderComponent.frameBuffers)
    {
        ResourceContext::DestroyResource(frameBuffer);
    }
}

void SceneRenderer::RegisterScene(Scene* scene_)
{
    EASY_FUNCTION()

    RemoveScene();

    scene = scene_;

    if (!scene->ctx().contains<CameraComponent&>())
    {
        Details::EmplaceDefaultCamera(*scene);
    }

    if (!scene->ctx().contains<EnvironmentComponent&>())
    {
        Details::EmplaceDefaultEnvironment(*scene);
    }

    scene->ctx().emplace<RenderContextComponent&>(renderComponent);

    if constexpr (Config::kRayTracingEnabled)
    {
        scene->ctx().emplace<RayTracingContextComponent&>(rayTracingComponent);
    }

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

    scene->ctx().erase<RayTracingContextComponent&>();

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

        if (scene->ctx().contains<RayTracingContextComponent>())
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

        rayTracingComponent.updated = false;
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
    if (extent.width == 0 || extent.height == 0)
    {
        return;
    }

    hybridRenderer->Resize(extent);

    if (pathTracingRenderer)
    {
        pathTracingRenderer->Resize(extent);
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
