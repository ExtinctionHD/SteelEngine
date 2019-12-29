#include "Engine/Render/RenderSystem.hpp"

RenderSystem::RenderSystem(const Window &window)
{
    vulkanContext = std::make_unique<VulkanContext>(window);
}

void RenderSystem::Process() const
{

}
