#include "Engine/Render/SceneRenderer.hpp"

#include "Engine/Config.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Components/StorageComponents.hpp"
#include "Engine/Scene/Components/TransformComponent.hpp"
#include "Engine/Render/HybridRenderer.hpp"
#include "Engine/Render/PathTracingRenderer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"
#include "Engine/Scene/Components/EnvironmentComponent.hpp"

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

    static RenderSceneComponent CreateRenderSceneComponent()
    {
        RenderSceneComponent renderComponent;

        renderComponent.lightBuffer = BufferHelpers::CreateEmptyBuffer(
                vk::BufferUsageFlagBits::eUniformBuffer, sizeof(gpu::Light) * MAX_LIGHT_COUNT);

        renderComponent.materialBuffer = BufferHelpers::CreateEmptyBuffer(
                vk::BufferUsageFlagBits::eUniformBuffer, sizeof(gpu::Material) * MAX_MATERIAL_COUNT);

        renderComponent.frameBuffers.resize(VulkanContext::swapchain->GetImageCount());

        for (auto& frameBuffer : renderComponent.frameBuffers)
        {
            frameBuffer = BufferHelpers::CreateEmptyBuffer(
                    vk::BufferUsageFlagBits::eUniformBuffer, sizeof(gpu::Frame));
        }

        renderComponent.updateLightBuffer = false;
        renderComponent.updateMaterialBuffer = false;

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

            if (lc.type == LightComponent::Type::eDirectional)
            {
                const glm::vec3 direction = tc.GetWorldTransform().GetAxis(Axis::eX);

                light.location = glm::vec4(-direction, 0.0f);
            }
            else if (lc.type == LightComponent::Type::ePoint)
            {
                const glm::vec3 position = tc.GetWorldTransform().GetTranslation();

                light.location = glm::vec4(position, 1.0f);
            }

            light.color = glm::vec4(lc.color, 0.0f);

            lights.push_back(light);
        }

        lights.resize(MAX_LIGHT_COUNT);

        const auto& renderComponent = scene.ctx().get<RenderSceneComponent>();

        BufferHelpers::UpdateBuffer(commandBuffer, renderComponent.lightBuffer, GetByteView(lights),
                SyncScope::kWaitForNone, SyncScope::kUniformRead);
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

        const auto& renderComponent = scene.ctx().get<RenderSceneComponent>();

        BufferHelpers::UpdateBuffer(commandBuffer, renderComponent.materialBuffer, GetByteView(materials),
                SyncScope::kWaitForNone, SyncScope::kUniformRead);
    }

    static void UpdateFrameBuffer(vk::CommandBuffer commandBuffer, const Scene& scene, uint32_t imageIndex)
    {
        const auto& renderComponent = scene.ctx().get<RenderSceneComponent>();
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

        BufferHelpers::UpdateBuffer(commandBuffer, renderComponent.frameBuffers[imageIndex],
                GetByteView(frameData), SyncScope::kWaitForNone, SyncScope::kUniformRead);
    }

    static RayTracingSceneComponent CreateRayTracingSceneComponent(const Scene& scene)
    {
        const auto& geometryComponent = scene.ctx().get<GeometryStorageComponent>();
        const auto& materialComponent = scene.ctx().get<MaterialStorageComponent>();

        RayTracingSceneComponent rayTracingComponent;

        for (auto&& [entity, tc, rc] : scene.view<TransformComponent, RenderComponent>().each())
        {
            vk::TransformMatrixKHR transformMatrix;

            const glm::mat4 transposedTransform = glm::transpose(tc.GetWorldTransform().GetMatrix());

            std::memcpy(&transformMatrix.matrix, &transposedTransform, sizeof(vk::TransformMatrixKHR));

            for (const auto& ro : rc.renderObjects)
            {
                Assert(ro.primitive <= static_cast<uint32_t>(std::numeric_limits<uint16_t>::max()));
                Assert(ro.material <= static_cast<uint32_t>(std::numeric_limits<uint8_t>::max()));

                const uint32_t customIndex = ro.primitive | (ro.material << 16);

                const Material& material = materialComponent.materials[ro.material];

                const vk::GeometryInstanceFlagsKHR flags = MaterialHelpers::GetTlasInstanceFlags(material.flags);

                const vk::AccelerationStructureKHR blas = geometryComponent.primitives[ro.primitive].GetBlas();

                rayTracingComponent.tlasInstances.push_back(vk::AccelerationStructureInstanceKHR(
                        transformMatrix, customIndex, 0xFF, 0, flags, VulkanContext::device->GetAddress(blas)));
            }
        }

        AccelerationStructureManager& accelerationStructureManager = *VulkanContext::accelerationStructureManager;

        rayTracingComponent.tlas = accelerationStructureManager.GenerateTlas(rayTracingComponent.tlasInstances); // TODO

        rayTracingComponent.buildTlas = true;

        return rayTracingComponent;
    }
}

SceneRenderer::SceneRenderer()
{
    hybridRenderer = std::make_unique<HybridRenderer>();

    if constexpr (Config::kRayTracingEnabled)
    {
        pathTracingRenderer = std::make_unique<PathTracingRenderer>();
    }

    renderSceneComponent = Details::CreateRenderSceneComponent();

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &SceneRenderer::HandleResizeEvent));

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &SceneRenderer::HandleKeyInputEvent));
}

SceneRenderer::~SceneRenderer()
{
    RemoveScene();

    if (renderSceneComponent.lightBuffer)
    {
        VulkanContext::bufferManager->DestroyBuffer(renderSceneComponent.lightBuffer);
    }

    VulkanContext::bufferManager->DestroyBuffer(renderSceneComponent.materialBuffer);

    for (const auto frameBuffer : renderSceneComponent.frameBuffers)
    {
        VulkanContext::bufferManager->DestroyBuffer(frameBuffer);
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

    scene->ctx().emplace<RenderSceneComponent&>(renderSceneComponent);

    renderSceneComponent.updateLightBuffer = true;
    renderSceneComponent.updateMaterialBuffer = true;

    if constexpr (Config::kRayTracingEnabled)
    {
        scene->ctx().emplace<RayTracingSceneComponent>(Details::CreateRayTracingSceneComponent(*scene));
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

    scene->ctx().erase<RenderSceneComponent>();

    if constexpr (Config::kRayTracingEnabled)
    {
        const auto& rayTracingComponent = scene->ctx().get<RayTracingSceneComponent>();

        VulkanContext::accelerationStructureManager->DestroyAccelerationStructure(rayTracingComponent.tlas);

        scene->ctx().erase<RayTracingSceneComponent>();
    }

    scene = nullptr;
}

void SceneRenderer::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    Details::UpdateFrameBuffer(commandBuffer, *scene, imageIndex);

    if (renderSceneComponent.updateLightBuffer)
    {
        Details::UpdateLightBuffer(commandBuffer, *scene);

        renderSceneComponent.updateLightBuffer = false;
    }

    if (renderSceneComponent.updateMaterialBuffer)
    {
        Details::UpdateMaterialBuffer(commandBuffer, *scene);

        renderSceneComponent.updateMaterialBuffer = false;
    }

    if constexpr (Config::kRayTracingEnabled)
    {
        auto& rayTracingComponent = scene->ctx().get<RayTracingSceneComponent>();

        if (!rayTracingComponent.tlasInstances.empty())
        {
            if (rayTracingComponent.buildTlas)
            {
                VulkanContext::accelerationStructureManager->BuildTlas(commandBuffer,
                        rayTracingComponent.tlas, rayTracingComponent.tlasInstances);
            }
            else
            {
                VulkanContext::accelerationStructureManager->UpdateTlas(commandBuffer,
                        rayTracingComponent.tlas, rayTracingComponent.tlasInstances);
            }

            rayTracingComponent.tlasInstances.clear();

            rayTracingComponent.buildTlas = false;
        }
    }

    if (renderMode == RenderMode::ePathTracing && pathTracingRenderer)
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
