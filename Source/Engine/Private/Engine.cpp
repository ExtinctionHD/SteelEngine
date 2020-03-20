#include "Engine/Engine.hpp"

#include "Engine/Config.hpp"
#include "Engine/EngineHelpers.hpp"
#include "Engine/CameraSystem.hpp"
#include "Engine/Render/UIRenderSystem.hpp"
#include "Engine/Render/RenderSystem.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"

namespace SEngine
{
    CameraProperties GetCameraProperties(const vk::Extent2D &extent)
    {
        return CameraProperties{
            glm::vec3(0.0f, 0.0f, -10.0f),
            Direction::kFront,
            Direction::kUp,
            90.0f, extent.width / static_cast<float>(extent.height),
            0.01f, 100.0f
        };
    }

    const CameraParameters kCameraParameters{
        1.0f, 1.0f, 2.0f
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

    camera = std::make_shared<Camera>(SEngine::GetCameraProperties(window->GetExtent()));

    vulkanContext = std::make_shared<VulkanContext>(GetRef(window));

    CameraSystem *cameraSystem = new CameraSystem(camera, SEngine::kCameraParameters, SEngine::kCameraKeyBindings);
    UIRenderSystem *uiRenderSystem = new UIRenderSystem(vulkanContext, GetRef(window));
    RenderSystem *renderSystem = new RenderSystem(vulkanContext, camera,
            MakeFunction(&UIRenderSystem::Render, uiRenderSystem));

    systems.emplace_back(uiRenderSystem);
    systems.emplace_back(renderSystem);
    systems.emplace_back(cameraSystem);
}

Engine::~Engine()
{
    vulkanContext->device->WaitIdle();
}

void Engine::Run() const
{
    while (!window->ShouldClose())
    {
        window->PollEvents();

        for (auto &system : systems)
        {
            system->Process(0.0f);
        }
    }
}

void Engine::ResizeCallback(const vk::Extent2D &extent) const
{
    vulkanContext->device->WaitIdle();

    if (extent.width > 0 && extent.height > 0)
    {
        const SwapchainInfo swapchainInfo{
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
