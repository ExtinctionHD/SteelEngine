#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

void BufferHelpers::InsertPipelineBarrier(
        vk::CommandBuffer commandBuffer, vk::Buffer buffer, const PipelineBarrier& barrier)
{
    const vk::BufferMemoryBarrier bufferMemoryBarrier(barrier.waitedScope.access, barrier.blockedScope.access,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, buffer, 0, VK_WHOLE_SIZE);

    commandBuffer.pipelineBarrier(barrier.waitedScope.stages, barrier.blockedScope.stages, vk::DependencyFlags(), {},
            { bufferMemoryBarrier }, {});
}

vk::Buffer BufferHelpers::CreateStagingBuffer(vk::DeviceSize size)
{
    const Queues::Description& queuesDescription = VulkanContext::device->GetQueuesDescription();

    const vk::BufferCreateInfo createInfo{
        {},
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::SharingMode::eExclusive,
        0,
        &queuesDescription.graphicsFamilyIndex,
    };

    const vk::MemoryPropertyFlags memoryProperties =
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    return VulkanContext::memoryManager->CreateBuffer(createInfo, memoryProperties);
}

void BufferHelpers::UpdateBuffer(vk::CommandBuffer commandBuffer, vk::Buffer buffer, const ByteView& data,
        const SyncScope& waitedScope, const SyncScope& blockedScope)
{
    {
        const PipelineBarrier barrier{
            waitedScope,
            SyncScope::kTransferWrite,
        };

        BufferHelpers::InsertPipelineBarrier(commandBuffer, buffer, barrier);
    }

    VulkanContext::bufferManager->UpdateBuffer(commandBuffer, buffer, data);

    {
        const PipelineBarrier barrier{
            SyncScope::kTransferWrite,
            blockedScope,
        };

        BufferHelpers::InsertPipelineBarrier(commandBuffer, buffer, barrier);
    }
}

void BufferHelpers::UpdateBuffer(vk::CommandBuffer commandBuffer, vk::Buffer buffer, const BufferUpdater& updater,
        const SyncScope& waitedScope, const SyncScope& blockedScope)
{
    {
        const PipelineBarrier barrier{
            waitedScope,
            SyncScope::kTransferWrite,
        };

        BufferHelpers::InsertPipelineBarrier(commandBuffer, buffer, barrier);
    }

    VulkanContext::bufferManager->UpdateBuffer(commandBuffer, buffer, updater);

    {
        const PipelineBarrier barrier{
            SyncScope::kTransferWrite,
            blockedScope,
        };

        BufferHelpers::InsertPipelineBarrier(commandBuffer, buffer, barrier);
    }
}

vk::Buffer BufferHelpers::CreateBufferWithData(vk::BufferUsageFlags usage, const ByteView& data)
{
    const BufferDescription bufferDescription{
        data.size,
        usage | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
    };

    const vk::Buffer buffer =
            VulkanContext::bufferManager->CreateBuffer(bufferDescription, BufferCreateFlagBits::eStagingBuffer);

    VulkanContext::device->ExecuteOneTimeCommands(
            [&](vk::CommandBuffer commandBuffer)
            {
                VulkanContext::bufferManager->UpdateBuffer(commandBuffer, buffer, data);
            });

    return buffer;
}

vk::Buffer BufferHelpers::CreateEmptyBuffer(vk::BufferUsageFlags usage, size_t size)
{
    const BufferDescription bufferDescription{
        size,
        usage | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
    };

    const vk::Buffer buffer =
            VulkanContext::bufferManager->CreateBuffer(bufferDescription, BufferCreateFlagBits::eStagingBuffer);

    return buffer;
}
