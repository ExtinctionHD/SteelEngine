#include "Engine/Render/RenderSystem.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

RenderSystem::RenderSystem()
{
    VulkanContext::Get();
}

void RenderSystem::Process() const
{
    
}
