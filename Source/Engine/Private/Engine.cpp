#include "Engine/Engine.hpp"

#include "Engine/Config.hpp"
#include "Engine/Filesystem/Filesystem.hpp"
#include "Engine/Scene/SceneModel.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/SceneRT.hpp"
#include "Engine/System/CameraSystem.hpp"
#include "Engine/System/UIRenderSystem.hpp"
#include "Engine/System/RenderSystemRT.hpp"
#include "Engine/System/RenderSystem.hpp"
#include "Engine/Render/FrameLoop.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace Details
{
    Filepath GetScenePath()
    {
        const DialogDescription dialogDescription{
            "Select Scene File", Filepath("~/"),
            { "glTF Files", "*.gltf" }
        };

        const std::optional<Filepath> scenePath = Filesystem::ShowOpenDialog(dialogDescription);

        return scenePath.value_or(Config::kDefaultScenePath);
    }

    Filepath GetEnvironmentPath()
    {
        const DialogDescription dialogDescription{
            "Select Environment File", Filepath("~/"),
            { "Image Files", "*.hdr *.png" }
        };

        const std::optional<Filepath> environmentPath = Filesystem::ShowOpenDialog(dialogDescription);

        return environmentPath.value_or(Config::kDefaultEnvironmentPath);
    }

    std::unique_ptr<Camera> CreateCamera(const vk::Extent2D& extent)
    {
        const float cameraAspectRation = extent.width / static_cast<float>(extent.height);
        const Camera::Description cameraDescription{
            Direction::kBackward * 3.0f,
            Direction::kForward,
            Direction::kUp,
            90.0f, cameraAspectRation,
            0.01f, 1000.0f
        };

        return std::make_unique<Camera>(cameraDescription);
    }
}

Timer Engine::timer;
Engine::State Engine::state;

std::unique_ptr<Window> Engine::window;
std::unique_ptr<FrameLoop> Engine::frameLoop;
std::unique_ptr<SceneModel> Engine::sceneModel;
std::unique_ptr<SceneRT> Engine::sceneRT;
std::unique_ptr<Scene> Engine::scene;

std::vector<std::unique_ptr<System>> Engine::systems;
std::map<EventType, std::vector<EventHandler>> Engine::eventMap;

void Engine::Create()
{
    window = std::make_unique<Window>(Config::kExtent, Config::kWindowMode);

    VulkanContext::Create(*window);

    frameLoop = std::make_unique<FrameLoop>();
    sceneModel = std::make_unique<SceneModel>(Details::GetScenePath());
    sceneRT = sceneModel->CreateSceneRT(Details::GetEnvironmentPath());
    scene = sceneModel->CreateScene(Details::GetEnvironmentPath());

    AddEventHandler<vk::Extent2D>(EventType::eResize, &Engine::HandleResizeEvent);

    AddSystem<CameraSystem>(sceneRT->GetCamera());
    AddSystem<UIRenderSystem>(*window);
    AddSystem<RenderSystemRT>(sceneRT.get());
    AddSystem<RenderSystem>(scene.get());
}

void Engine::Run()
{
    while (!window->ShouldClose())
    {
        window->PollEvents();

        for (auto& system : systems)
        {
            system->Process(timer.GetDeltaSeconds());
        }

        if (state.drawingSuspended)
        {
            continue;
        }

        frameLoop->Draw([](vk::CommandBuffer commandBuffer, uint32_t imageIndex)
            {
                if (state.rayTracingMode)
                {
                    GetSystem<RenderSystemRT>()->Render(commandBuffer, imageIndex);
                }
                else
                {
                    GetSystem<RenderSystem>()->Render(commandBuffer, imageIndex);
                }

                GetSystem<UIRenderSystem>()->Render(commandBuffer, imageIndex);
            });
    }
}

void Engine::Destroy()
{
    VulkanContext::device->WaitIdle();

    systems.clear();

    scene.reset();
    sceneRT.reset();
    sceneModel.reset();
    frameLoop.reset();
    window.reset();

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
    }
}
