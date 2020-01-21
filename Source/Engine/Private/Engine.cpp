#include "Engine/Engine.hpp"

#include "Utils/Helpers.hpp"

void Engine::Run() const
{
    while (!window->ShouldClose())
    {
        window->PollEvents();
        renderSystem->Draw();
    }
}

Engine::Engine()
{
    window = std::make_unique<Window>(vk::Extent2D(1280, 720), Window::eMode::kWindowed);
    renderSystem = std::make_unique<RenderSystem>(GetRef(window));
}
