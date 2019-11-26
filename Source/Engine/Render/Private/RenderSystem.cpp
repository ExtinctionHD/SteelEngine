#include "Engine/Render/RenderSystem.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

void RenderSystem::ConnectWindow(GLFWwindow *window)
{
    VulkanContext::Get()->CreateSurface(window);
}

void RenderSystem::Process() const
{
    
}
