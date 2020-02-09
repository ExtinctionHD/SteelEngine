#include "Engine/Engine.hpp"

#include "Engine/Config.hpp"

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
    window = std::make_unique<Window>(Config::kExtent, Config::kMode);
    renderSystem = std::make_unique<RenderSystem>(GetRef(window));
}
