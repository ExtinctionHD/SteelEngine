#include "Engine/Render/FrameLoop.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"

namespace Details
{
    static CommandBufferSync CreateCommandBufferSync()
    {
        const vk::Device device = VulkanContext::device->Get();

        CommandBufferSync commandBufferSync;
        commandBufferSync.waitSemaphores.push_back(VulkanHelpers::CreateSemaphore(device));
        commandBufferSync.waitStages.emplace_back(vk::PipelineStageFlagBits::eComputeShader);
        commandBufferSync.signalSemaphores.push_back(VulkanHelpers::CreateSemaphore(device));
        commandBufferSync.fence = VulkanHelpers::CreateFence(device, vk::FenceCreateFlagBits::eSignaled);

        return commandBufferSync;
    }

    static uint32_t AcquireNextImageIndex(vk::Semaphore signalSemaphore)
    {
        const vk::Device device = VulkanContext::device->Get();
        const vk::SwapchainKHR swapchain = VulkanContext::swapchain->Get();

        const auto& [result, imageIndex] = device.acquireNextImageKHR(swapchain, Numbers::kMaxUint, signalSemaphore);

        Assert(result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR);

        return imageIndex;
    }

    static void WaitAndResetFence(vk::Fence fence)
    {
        const vk::Device device = VulkanContext::device->Get();

        VulkanHelpers::WaitForFences(device, { fence });

        const vk::Result resetResult = device.resetFences(fence);
        Assert(resetResult == vk::Result::eSuccess);
    }

    static void PresentImage(vk::Queue presentQueue, uint32_t imageIndex, vk::Semaphore waitSemaphore)
    {
        const vk::SwapchainKHR swapchain = VulkanContext::swapchain->Get();

        const vk::PresentInfoKHR presentInfo(waitSemaphore, swapchain, imageIndex, nullptr);

        const vk::Result result = presentQueue.presentKHR(presentInfo);
        Assert(result == vk::Result::eSuccess);
    }
}

FrameLoop::FrameLoop()
{
    frames.resize(VulkanContext::swapchain->GetImageCount());

    for (auto& frame : frames)
    {
        frame.commandBuffer = VulkanContext::device->AllocateCommandBuffer(CommandBufferType::eOneTime);
        frame.commandBufferSync = Details::CreateCommandBufferSync();
    }
}

FrameLoop::~FrameLoop()
{
    for (const ResourceToDestroy& resourceToDestroy : resourcesToDestroy)
    {
        resourceToDestroy.destroyTask();
    }

    for (const auto& frame : frames)
    {
        VulkanHelpers::DestroyCommandBufferSync(VulkanContext::device->Get(), frame.commandBufferSync);
    }
}

uint32_t FrameLoop::GetFrameCount() const
{
    return static_cast<uint32_t>(frames.size());
}

bool FrameLoop::IsFrameActive(uint32_t frameIndex) const
{
    Assert(frameIndex < GetFrameCount());

    const vk::Fence fence = frames[frameIndex].commandBufferSync.fence;

    return VulkanContext::device->Get().getFenceStatus(fence) == vk::Result::eNotReady;
}

void FrameLoop::Draw(RenderCommands renderCommands)
{
    const auto& [graphicsQueue, presentQueue] = VulkanContext::device->GetQueues();
    const auto& [commandBuffer, commandBufferSync] = frames[currentFrameIndex];

    const uint32_t imageIndex = Details::AcquireNextImageIndex(commandBufferSync.waitSemaphores.front());

    Details::WaitAndResetFence(commandBufferSync.fence);

    UpdateResourcesToDestroy();

    const DeviceCommands deviceCommands = [&](vk::CommandBuffer cb)
    {
        renderCommands(cb, imageIndex);
    };

    VulkanHelpers::SubmitCommandBuffer(graphicsQueue, commandBuffer, deviceCommands, commandBufferSync);

    Details::PresentImage(presentQueue, imageIndex, commandBufferSync.signalSemaphores.front());

    currentFrameIndex = (currentFrameIndex + 1) % frames.size();
}

void FrameLoop::DestroyResource(std::function<void()>&& destroyTask)
{
    std::set<uint32_t> framesToWait;

    for (uint32_t i = 0; i < GetFrameCount(); ++i)
    {
        if (IsFrameActive(i))
        {
            framesToWait.insert(i);
        }
    }

    resourcesToDestroy.push_back(ResourceToDestroy{ destroyTask, std::move(framesToWait) });
}

void FrameLoop::UpdateResourcesToDestroy()
{
    for (ResourceToDestroy& resourceToDestroy : resourcesToDestroy)
    {
        std::erase_if(resourceToDestroy.framesToWait,
                [&](uint32_t frameIndex)
                {
                    return !IsFrameActive(frameIndex);
                });

        if (resourceToDestroy.framesToWait.empty())
        {
            resourceToDestroy.destroyTask();
        }
    }

    std::erase_if(resourcesToDestroy,
            [](const ResourceToDestroy& resourceToDestroy)
            {
                return resourceToDestroy.framesToWait.empty();
            });
}
