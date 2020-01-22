#include "Engine/Render/RenderSystem.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Helpers.hpp"

RenderSystem::RenderSystem(const Window &window)
{
    vulkanContext = std::make_unique<VulkanContext>(window);

    for (auto &frame : frames)
    {
        frame.commandBuffer = vulkanContext->device->AllocateCommandBuffer();
        frame.presentCompleteSemaphore = VulkanHelpers::CreateSemaphore(GetRef(vulkanContext->device));
        frame.renderCompleteSemaphore = VulkanHelpers::CreateSemaphore(GetRef(vulkanContext->device));
    }
}

RenderSystem::~RenderSystem()
{
    for (auto& frame : frames)
    {
        vulkanContext->device->Get().destroyFramebuffer(frame.framebuffer);
        vulkanContext->device->Get().destroySemaphore(frame.presentCompleteSemaphore);
        vulkanContext->device->Get().destroySemaphore(frame.renderCompleteSemaphore);
    }
}

void RenderSystem::Process() const
{ }

void RenderSystem::Draw() const
{ }
