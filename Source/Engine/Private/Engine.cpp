#include "Engine/Engine.hpp"

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
    window = std::make_unique<Window>(1280, 720, Window::eMode::kWindowed);
    renderSystem = std::make_unique<RenderSystem>(window->Get());
}

void Engine::ProcessSystems() const
{
    renderSystem->Process();
}
