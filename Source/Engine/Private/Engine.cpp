#include "Engine/Engine.hpp"

#include "Engine/Config.hpp"
#include "Engine/Render/UIRenderSystem.hpp"
#include "Engine/Render/RenderSystem.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"

Engine::Engine()
{
    window = std::make_unique<Window>(Config::kExtent, Config::kMode);
    window->SetResizeCallback([this](const vk::Extent2D &extent)
        {
            WindowResizeCallback(extent);
        });

    vulkanContext = std::make_shared<VulkanContext>(GetRef(window));

    UIRenderSystem *uiRenderSystem = new UIRenderSystem(vulkanContext, GetRef(window));
    RenderSystem *renderSystem = new RenderSystem(vulkanContext, uiRenderSystem->GetUIRenderFunction());

    systems.emplace_back(uiRenderSystem);
    systems.emplace_back(renderSystem);
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

void Engine::WindowResizeCallback(const vk::Extent2D &extent)
{
    vulkanContext->device->WaitIdle();

    if (extent.width > 0 && extent.height > 0)
    {
        vulkanContext->swapchain->Recreate({ vulkanContext->surface->Get(), extent });
    }

    for (auto &system : systems)
    {
        system->OnResize(extent);
    }
}
