#include "Engine/Engine.hpp"

#include "Engine/Render/Vulkan/Vulkan.hpp"

#include "Utils/Helpers.hpp"

void Engine::Run() const
{
    while (!window->ShouldClose())
    {
        window->PollEvents();

        ProcessSystems();
    }
}

Engine::Engine()
{
    window = std::make_unique<Window>(vk::Extent2D(1280, 720), Window::eMode::kWindowed);
    renderSystem = std::make_unique<RenderSystem>(GetRef(window));
}

void Engine::ProcessSystems() const
{
    renderSystem->Process();
}
