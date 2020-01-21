#include "Engine/Render/RenderLoop.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Helpers.hpp"

RenderLoop::RenderLoop(std::shared_ptr<Device> aDevice, uint32_t frameCount)
    : device(aDevice)
    , frames(frameCount)
{
    for (auto &frameData : frames)
    {
        frameData.commandBuffer = device->AllocateCommandBuffer();
        frameData.presentCompleteSemaphore = VulkanHelpers::CreateSemaphore(GetRef(device));
        frameData.renderCompleteSemaphore = VulkanHelpers::CreateSemaphore(GetRef(device));
    }
}

RenderLoop::~RenderLoop()
{
    for (auto &frameData : frames)
    {
        device->Get().destroySemaphore(frameData.presentCompleteSemaphore);
        device->Get().destroySemaphore(frameData.renderCompleteSemaphore);
    }
}

void RenderLoop::ProcessNextFrame(const ForwardRenderPass &renderPass)
{
}
