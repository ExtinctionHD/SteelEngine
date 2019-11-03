#include "Engine/Engine.hpp"

Engine *Engine::Instance()
{
    if (instance == nullptr)
    {
        instance = new Engine();
    }

    return instance;
}

void Engine::Run()
{
    window = std::make_unique<Window>(1280, 720, Window::eMode::kWindowed);

    while(!window->ShouldClose())
    {
        window->PollEvents();
    }
}
