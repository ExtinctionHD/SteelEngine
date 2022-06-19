#include "Engine2/Engine2.hpp"

#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/FrameLoop.hpp"
#include "Engine/Window.hpp"
#include "Engine2/Scene2.hpp"

using namespace Steel;

std::unique_ptr<Window> Engine2::window;

void Engine2::Create()
{
    window = std::make_unique<Window>(Config::kExtent, Config::kWindowMode);

    VulkanContext::Create(*window);
    RenderContext::Create();
}

void Engine2::Run()
{
    while (!window->ShouldClose())
    {
        window->PollEvents();
    }
}

void Engine2::Destroy()
{
    RenderContext::Destroy();
    VulkanContext::Destroy();

    window.reset();
}
