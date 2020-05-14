#include "Engine/Engine.hpp"

#include "Engine/Config.hpp"
#include "Engine/CameraSystem.hpp"
#include "Engine/Scene/SceneLoader.hpp"
#include "Engine/Render/UIRenderSystem.hpp"
#include "Engine/Render/RenderSystem.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Filesystem/Filesystem.hpp"
#include "Engine/EngineHelpers.hpp"

namespace SEngine
{
    const CameraSystem::Parameters kCameraParameters{
        1.0f, 2.0f, 4.0f
    };

    const CameraSystem::MovementKeyBindings kCameraMovementKeyBindings{
        { CameraSystem::MovementAxis::eForward, { Key::eW, Key::eS } },
        { CameraSystem::MovementAxis::eLeft, { Key::eA, Key::eD } },
        { CameraSystem::MovementAxis::eUp, { Key::eSpace, Key::eLeftControl } },
    };

    const CameraSystem::SpeedKeyBindings kCameraSpeedKeyBindings{
        Key::e1, Key::e2, Key::e3, Key::e4, Key::e5
    };

    const Filepath kDefaultScene = Filepath("~/Assets/Scenes/Helmets/Helmets.gltf");

    std::unique_ptr<Scene> LoadScene()
    {
        const DialogDescription description{
            "Select Scene File",
            Filepath("~/"),
            { "glTF Files", "*.gltf" }
        };

        const std::optional<Filepath> sceneFile = Filesystem::ShowOpenDialog(description);

        return SceneLoader::LoadFromFile(Filepath(sceneFile.value_or(kDefaultScene)));
    }

    Camera::Description GetCameraInfo(const vk::Extent2D &extent)
    {
        return Camera::Description{
            Direction::kBackward * 3.0f,
            Direction::kForward,
            Direction::kUp,
            90.0f, extent.width / static_cast<float>(extent.height),
            0.01f, 1000.0f
        };
    }
}

Timer Engine::timer;

std::unique_ptr<Window> Engine::window;
std::unique_ptr<Scene> Engine::scene;
std::unique_ptr<Camera> Engine::camera;
std::vector<std::unique_ptr<System>> Engine::systems;

void Engine::Create()
{
    window = std::make_unique<Window>(Config::kExtent, Config::kWindowMode);
    window->SetResizeCallback(&Engine::ResizeCallback);
    window->SetKeyInputCallback(&Engine::KeyInputCallback);
    window->SetMouseInputCallback(&Engine::MouseInputCallback);
    window->SetMouseMoveCallback(&Engine::MouseMoveCallback);

    VulkanContext::Create(*window);

    camera = std::make_unique<Camera>(SEngine::GetCameraInfo(window->GetExtent()));
    scene = SEngine::LoadScene();

    AddSystem<CameraSystem>(camera.get(), SEngine::kCameraParameters,
            SEngine::kCameraMovementKeyBindings, SEngine::kCameraSpeedKeyBindings);

    AddSystem<UIRenderSystem>(*window);

    AddSystem<RenderSystem>(scene.get(), camera.get(),
            MakeFunction(&UIRenderSystem::Render, GetSystem<UIRenderSystem>()));
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
    }

    VulkanContext::device->WaitIdle();
}

void Engine::ResizeCallback(const vk::Extent2D &extent)
{
    VulkanContext::device->WaitIdle();

    if (extent.width > 0 && extent.height > 0)
    {
        const Swapchain::Description description{
            extent, Config::kVSyncEnabled
        };

        VulkanContext::swapchain->Recreate(description);
    }

    for (auto &system : systems)
    {
        system->OnResize(extent);
    }
}

void Engine::KeyInputCallback(Key key, KeyAction action, ModifierFlags modifiers)
{
    for (auto &system : systems)
    {
        system->OnKeyInput(key, action, modifiers);
    }
}

void Engine::MouseInputCallback(MouseButton button, MouseButtonAction action, ModifierFlags modifiers)
{
    for (auto &system : systems)
    {
        system->OnMouseInput(button, action, modifiers);
    }
}

void Engine::MouseMoveCallback(const glm::vec2 &position)
{
    for (auto &system : systems)
    {
        system->OnMouseMove(position);
    }
}
