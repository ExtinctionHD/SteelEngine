#include "Engine/Render/RenderSystem.hpp"

#include "Utils/Helpers.hpp"

RenderSystem::RenderSystem(const Window &window)
{
    vulkanContext = std::make_unique<VulkanContext>(window);
}

void RenderSystem::Process() const
{ }

void RenderSystem::Draw() const
{
    renderLoop->ProcessNextFrame(GetRef(renderPass));
}
