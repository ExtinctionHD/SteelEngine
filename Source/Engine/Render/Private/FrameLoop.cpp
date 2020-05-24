#include "Engine/Render/FrameLoop.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"

FrameLoop::FrameLoop()
{
    frames.resize(VulkanContext::swapchain->GetImageViews().size());
    for (auto &frame : frames)
    {
        frame.commandBuffer = VulkanContext::device->AllocateCommandBuffer(CommandBufferType::eOneTime);
        frame.sync.waitSemaphores.push_back(VulkanHelpers::CreateSemaphore(VulkanContext::device->Get()));
        frame.sync.signalSemaphores.push_back(VulkanHelpers::CreateSemaphore(VulkanContext::device->Get()));
        frame.sync.fence = VulkanHelpers::CreateFence(VulkanContext::device->Get(), vk::FenceCreateFlagBits::eSignaled);
        frame.sync.waitStages.emplace_back(vk::PipelineStageFlagBits::eRayTracingShaderNV);
    }
}

FrameLoop::~FrameLoop()
{
    for (auto &frame : frames)
    {
        VulkanHelpers::DestroyCommandBufferSync(VulkanContext::device->Get(), frame.sync);
    }
}

void FrameLoop::Draw(RenderCommands renderCommands)
{
    const vk::SwapchainKHR swapchain = VulkanContext::swapchain->Get();
    const vk::Device device = VulkanContext::device->Get();

    const auto &[graphicsQueue, presentQueue] = VulkanContext::device->GetQueues();
    const auto &[commandBuffer, synchronization] = frames[frameIndex];

    const vk::Semaphore presentCompleteSemaphore = synchronization.waitSemaphores.front();
    const vk::Semaphore renderingCompleteSemaphore = synchronization.signalSemaphores.front();
    const vk::Fence renderingFence = synchronization.fence;

    const auto &[acquireResult, imageIndex] = device.acquireNextImageKHR(
            swapchain, Numbers::kMaxUint, presentCompleteSemaphore, nullptr);

    if (acquireResult == vk::Result::eErrorOutOfDateKHR) return;
    Assert(acquireResult == vk::Result::eSuccess || acquireResult == vk::Result::eSuboptimalKHR);

    VulkanHelpers::WaitForFences(device, { renderingFence });

    const vk::Result resetResult = device.resetFences(1, &renderingFence);
    Assert(resetResult == vk::Result::eSuccess);

    const DeviceCommands deviceCommands = std::bind(renderCommands, std::placeholders::_1, imageIndex);
    VulkanHelpers::SubmitCommandBuffer(graphicsQueue, commandBuffer, deviceCommands, synchronization);

    const vk::PresentInfoKHR presentInfo(1, &renderingCompleteSemaphore,
            1, &swapchain, &imageIndex, nullptr);

    const vk::Result presentResult = presentQueue.presentKHR(presentInfo);
    Assert(presentResult == vk::Result::eSuccess);

    frameIndex = (frameIndex + 1) % frames.size();
}
