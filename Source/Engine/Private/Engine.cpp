#include "Engine/Engine.hpp"

#include "Engine/Config.hpp"
#include "Engine/EngineHelpers.hpp"
#include "Engine/CameraSystem.hpp"
#include "Engine/Render/UIRenderSystem.hpp"
#include "Engine/Render/RenderSystem.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"

namespace SEngine
{
    CameraDescription GetCameraInfo(const vk::Extent2D &extent)
    {
        return CameraDescription{
            glm::vec3(0.0f, 0.0f, -5.0f),
            Direction::kForward,
            Direction::kUp,
            90.0f, extent.width / static_cast<float>(extent.height),
            0.01f, 100.0f
        };
    }

    const CameraParameters kCameraParameters{
        1.0f, 2.0f, 6.0f
    };

    const CameraKeyBindings kCameraKeyBindings{
        Key::eW, Key::eS, Key::eA, Key::eD,
        Key::eSpace, Key::eLeftControl, Key::eLeftShift
    };
}

Engine::Engine()
{
    window = std::make_unique<Window>(Config::kExtent, Config::kMode);
    window->SetResizeCallback(MakeFunction(&Engine::ResizeCallback, this));
    window->SetKeyInputCallback(MakeFunction(&Engine::KeyInputCallback, this));
    window->SetMouseInputCallback(MakeFunction(&Engine::MouseInputCallback, this));
    window->SetMouseMoveCallback(MakeFunction(&Engine::MouseMoveCallback, this));

    camera = std::make_shared<Camera>(SEngine::GetCameraInfo(window->GetExtent()));

    vulkanContext = std::make_shared<VulkanContext>(GetRef(window));

    systems.emplace_back(new CameraSystem(camera, SEngine::kCameraParameters, SEngine::kCameraKeyBindings));
    systems.emplace_back(new UIRenderSystem(vulkanContext, GetRef(window)));

    UIRenderSystem *uiRenderSystem = dynamic_cast<UIRenderSystem *>(systems.back().get());
    const RenderFunction uiRenderFunction = MakeFunction(&UIRenderSystem::Render, uiRenderSystem);
    systems.emplace_back(new RenderSystem(vulkanContext, camera, uiRenderFunction));

    sceneLoader = std::make_unique<SceneLoader>(vulkanContext->bufferManager, vulkanContext->textureCache);
    scene = sceneLoader->LoadFromFile(Filepath("~/Assets/Scenes/BoxTextured/BoxTextured.gltf"));
}

Engine::~Engine()
{
    vulkanContext->device->WaitIdle();
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
}

void Engine::ResizeCallback(const vk::Extent2D &extent) const
{
    vulkanContext->device->WaitIdle();

    if (extent.width > 0 && extent.height > 0)
    {
        const SwapchainDescription swapchainInfo{
            vulkanContext->surface->Get(), extent, Config::kVSyncEnabled
        };

        vulkanContext->swapchain->Recreate(swapchainInfo);
    }

    for (auto &system : systems)
    {
        system->OnResize(extent);
    }
}

void Engine::KeyInputCallback(Key key, KeyAction action, ModifierFlags modifiers) const
{
    for (auto &system : systems)
    {
        system->OnKeyInput(key, action, modifiers);
    }
}

void Engine::MouseInputCallback(MouseButton button, MouseButtonAction action, ModifierFlags modifiers) const
{
    for (auto &system : systems)
    {
        system->OnMouseInput(button, action, modifiers);
    }
}

void Engine::MouseMoveCallback(const glm::vec2 &position) const
{
    for (auto &system : systems)
    {
        system->OnMouseMove(position);
    }
}
