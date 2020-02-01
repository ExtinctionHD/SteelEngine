#include "Engine/Engine.hpp"

#include "Utils/Helpers.hpp"

void Engine::Run() const
{
    while (!window->ShouldClose())
    {
        window->PollEvents();
        renderSystem->Process();
        renderSystem->Draw();
    }
}

Engine::Engine()
{
    window = std::make_unique<Window>(vk::Extent2D(1920, 1080), Window::eMode::kWindowed);
    renderSystem = std::make_unique<RenderSystem>(GetRef(window));
}
