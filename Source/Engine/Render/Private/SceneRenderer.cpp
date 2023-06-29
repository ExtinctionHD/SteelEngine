#include "Engine/Render/SceneRenderer.hpp"

#include "Engine/Config.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Scene/StorageComponents.hpp"
#include "Engine/Render/HybridRenderer.hpp"
#include "Engine/Render/PathTracingRenderer.hpp"
#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

namespace Details
{
    static void UpdateFrameBuffer(vk::CommandBuffer commandBuffer, const Scene& scene, uint32_t imageIndex)
    {
        const auto& renderComponent = scene.ctx().get<RenderStorageComponent>();
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
}

SceneRenderer::SceneRenderer()
{
    hybridRenderer = std::make_unique<HybridRenderer>();

    if constexpr (Config::kRayTracingEnabled)
    {
        pathTracingRenderer = std::make_unique<PathTracingRenderer>();
    }

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &SceneRenderer::HandleResizeEvent));

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &SceneRenderer::HandleKeyInputEvent));
}

SceneRenderer::~SceneRenderer() = default;

void SceneRenderer::RegisterScene(const Scene* scene_)
{
    EASY_FUNCTION()

    RemoveScene();

    scene = scene_;

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

    scene = nullptr;
}

void SceneRenderer::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    Details::UpdateFrameBuffer(commandBuffer, *scene, imageIndex);

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
