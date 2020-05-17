#include "Engine/Engine.hpp"

#include "Engine/Config.hpp"
#include "Engine/Scene/SceneLoader.hpp"
#include "Engine/Render/FrameLoop.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Filesystem/Filesystem.hpp"
#include "Engine/System/CameraSystem.hpp"
#include "Engine/System/UIRenderSystem.hpp"
#include "Engine/System/PathTracingSystem.hpp"

namespace SEngine
{
    std::unique_ptr<Scene> LoadScene()
    {
        const DialogDescription description{
            "Select Scene File", Filepath("~/"),
            { "glTF Files", "*.gltf" }
        };

        const std::optional<Filepath> sceneFile = Filesystem::ShowOpenDialog(description);

        return SceneLoader::LoadFromFile(Filepath(sceneFile.value_or(Config::kDefaultScenePath)));
    }

    std::unique_ptr<Camera> CreateCamera(const vk::Extent2D &extent)
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
std::unique_ptr<Scene> Engine::scene;
std::unique_ptr<Camera> Engine::camera;
std::unique_ptr<FrameLoop> Engine::frameLoop;

std::vector<std::unique_ptr<System>> Engine::systems;
std::map<EventType, std::vector<EventHandler>> Engine::eventMap;

void Engine::Create()
{
    window = std::make_unique<Window>(Config::kExtent, Config::kWindowMode);

    VulkanContext::Create(*window);

    scene = SEngine::LoadScene();
    camera = SEngine::CreateCamera(window->GetExtent());
    frameLoop = std::make_unique<FrameLoop>();

    AddEventHandler<vk::Extent2D>(EventType::eResize, &Engine::HandleResizeEvent);

    AddSystem<CameraSystem>(camera.get());
    AddSystem<UIRenderSystem>(*window);
    AddSystem<PathTracingSystem>(scene.get(), camera.get());
}

void Engine::Run()
{
    while (!window->ShouldClose())
    {
        window->PollEvents();

        for (auto &system : systems)
        {
            system->Process(timer.GetDeltaSeconds());
        }

        if (state.drawingSuspended)
        {
            continue;
        }

        frameLoop->Draw([](vk::CommandBuffer commandBuffer, uint32_t imageIndex)
            {
                GetSystem<PathTracingSystem>()->Render(commandBuffer, imageIndex);
                GetSystem<UIRenderSystem>()->Render(commandBuffer, imageIndex);
            });
    }
}

void Engine::Destroy()
{
    VulkanContext::device->WaitIdle();

    systems.clear();

    frameLoop.reset(nullptr);
    camera.reset(nullptr);
    scene.reset(nullptr);
    window.reset(nullptr);

    VulkanContext::Destroy();
}

void Engine::TriggerEvent(EventType type)
{
    for (const auto &handler : eventMap[type])
    {
        handler(std::any());
    }
}

void Engine::AddEventHandler(EventType type, std::function<void()> handler)
{
    std::vector<EventHandler> &eventHandlers = eventMap[type];
    eventHandlers.emplace_back([handler](std::any)
        {
            handler();
        });
}

void Engine::HandleResizeEvent(const vk::Extent2D &extent)
{
    VulkanContext::device->WaitIdle();

    state.drawingSuspended = extent.width == 0 || extent.height == 0;

    if (!state.drawingSuspended)
    {
        const Swapchain::Description description{
            extent, Config::kVSyncEnabled
        };

        VulkanContext::swapchain->Recreate(description);
    }
}
