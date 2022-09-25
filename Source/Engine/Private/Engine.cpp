#include "Engine/Engine.hpp"

#include "Engine/Config.hpp"
#include "Engine/Filesystem/Filesystem.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Systems/CameraSystem.hpp"
#include "Engine/Systems/UIRenderSystem.hpp"
#include "Engine/Render/PathTracingRenderer.hpp"
#include "Engine/Render/HybridRenderer.hpp"
#include "Engine/Render/FrameLoop.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace Details
{
    static Filepath GetScenePath()
    {
        if constexpr (Config::kUseDefaultAssets)
        {
            return Config::kDefaultScenePath;
        }
        else
        {
            const DialogDescription dialogDescription{
                "Select Scene File", Filepath("~/"),
                { "glTF Files", "*.gltf" }
            };

            const std::optional<Filepath> scenePath = Filesystem::ShowOpenDialog(dialogDescription);

            return scenePath.value_or(Config::kDefaultScenePath);
        }
    }
}

Timer Engine::timer;
Engine::State Engine::state;

std::unique_ptr<Window> Engine::window;
std::unique_ptr<FrameLoop> Engine::frameLoop;

std::unique_ptr<Scene> Engine::scene;

std::unique_ptr<HybridRenderer> Engine::hybridRenderer;
std::unique_ptr<PathTracingRenderer> Engine::pathTracingRenderer;

std::vector<std::unique_ptr<System>> Engine::systems;
std::map<EventType, std::vector<EventHandler>> Engine::eventMap;

void Engine::Create()
{
    window = std::make_unique<Window>(Config::kExtent, Config::kWindowMode);

    VulkanContext::Create(*window);
    RenderContext::Create();

    AddEventHandler<vk::Extent2D>(EventType::eResize, &Engine::HandleResizeEvent);
    AddEventHandler<KeyInput>(EventType::eKeyInput, &Engine::HandleKeyInputEvent);
    AddEventHandler<MouseInput>(EventType::eMouseInput, &Engine::HandleMouseInputEvent);

    frameLoop = std::make_unique<FrameLoop>();

    hybridRenderer = std::make_unique<HybridRenderer>();

    OpenScene();

    if constexpr (Config::kRayTracingEnabled)
    {
        pathTracingRenderer = std::make_unique<PathTracingRenderer>(scene.get());
    }

    AddSystem<CameraSystem>();

    AddSystem<UIRenderSystem>(*window);
}

void Engine::Run()
{
    while (!window->ShouldClose())
    {
        window->PollEvents();

        if (scene)
        {
            const float deltaSeconds = timer.GetDeltaSeconds();

            for (const auto& system : systems)
            {
                system->Process(*scene, deltaSeconds);
            }
        }
        
        if (state.drawingSuspended)
        {
            continue;
        }

        frameLoop->Draw([](vk::CommandBuffer commandBuffer, uint32_t imageIndex)
            {
                if (state.renderMode == RenderMode::ePathTracing && pathTracingRenderer)
                {
                    pathTracingRenderer->Render(commandBuffer, imageIndex);
                }
                else
                {
                    hybridRenderer->Render(commandBuffer, imageIndex);
                }

                GetSystem<UIRenderSystem>()->Render(commandBuffer, imageIndex);
            });
    }
}

void Engine::Destroy()
{
    VulkanContext::device->WaitIdle();

    systems.clear();

    hybridRenderer.reset();
    pathTracingRenderer.reset();

    scene.reset();
    frameLoop.reset();
    window.reset();

    RenderContext::Destroy();
    VulkanContext::Destroy();
}

void Engine::TriggerEvent(EventType type)
{
    for (const auto& handler : eventMap[type])
    {
        handler(std::any());
    }
}

void Engine::AddEventHandler(EventType type, std::function<void()> handler)
{
    std::vector<EventHandler>& eventHandlers = eventMap[type];
    eventHandlers.emplace_back([handler](std::any)
        {
            handler();
        });
}

void Engine::HandleResizeEvent(const vk::Extent2D& extent)
{
    VulkanContext::device->WaitIdle();

    state.drawingSuspended = extent.width == 0 || extent.height == 0;

    if (!state.drawingSuspended)
    {
        const Swapchain::Description swapchainDescription{
            extent, Config::kVSyncEnabled
        };

        VulkanContext::swapchain->Recreate(swapchainDescription);

        hybridRenderer->Resize(extent);

        if (pathTracingRenderer)
        {
            pathTracingRenderer->Resize(extent);
        }
    }
}

void Engine::HandleKeyInputEvent(const KeyInput& keyInput)
{
    if (keyInput.action == KeyAction::ePress)
    {
        switch (keyInput.key)
        {
        case Key::eO:
            OpenScene();
            break;
        case Key::eT:
            ToggleRenderMode();
            break;
        default:
            break;
        }
    }
}

void Engine::HandleMouseInputEvent(const MouseInput& mouseInput)
{
    if (mouseInput.button == Config::DefaultCamera::kControlMouseButton)
    {
        switch (mouseInput.action)
        {
        case MouseButtonAction::ePress:
            window->SetCursorMode(Window::CursorMode::eDisabled);
            break;
        case MouseButtonAction::eRelease:
            window->SetCursorMode(Window::CursorMode::eEnabled);
            break;
        default:
            break;
        }
    }
}

void Engine::ToggleRenderMode()
{
    uint32_t i = static_cast<const uint32_t>(state.renderMode);

    i = (i + 1) % kRenderModeCount;

    state.renderMode = static_cast<RenderMode>(i);
}

void Engine::OpenScene()
{
    VulkanContext::device->WaitIdle();

    hybridRenderer->RemoveScene();

    scene = std::make_unique<Scene>(Details::GetScenePath());
    scene->PrepareToRender();

    hybridRenderer->RegisterScene(scene.get());
}
