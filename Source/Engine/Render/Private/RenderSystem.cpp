#include "Engine/Render/RenderSystem.hpp"

RenderSystem::RenderSystem(GLFWwindow *window)
{
    vulkanContext = std::make_unique<VulkanContext>(window);
}

void RenderSystem::Process() const
{

}
