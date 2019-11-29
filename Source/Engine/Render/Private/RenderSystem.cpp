#include "Engine/Render/RenderSystem.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

RenderSystem::RenderSystem()
{
    VulkanContext::Initialize();
}

void RenderSystem::SetupWindow(GLFWwindow *window)
{
    VulkanContext::Get()->CreateSurface(window);
}

void RenderSystem::Process() const
{

}
