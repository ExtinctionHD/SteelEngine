#include "Engine/Engine.hpp"

#include "Engine/Config.hpp"
#include "Engine/Render/UIRenderSystem.hpp"
#include "Engine/Render/RenderSystem.hpp"

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

Engine::Engine()
{
    window = std::make_unique<Window>(Config::kExtent, Config::kMode);
    vulkanContext = std::make_shared<VulkanContext>(GetRef(window));

    UIRenderSystem *uiRenderSystem = new UIRenderSystem(vulkanContext, GetRef(window));
    RenderSystem *renderSystem = new RenderSystem(vulkanContext, uiRenderSystem->GetUIRenderFunction());

    systems.emplace_back(uiRenderSystem);
    systems.emplace_back(renderSystem);
}
