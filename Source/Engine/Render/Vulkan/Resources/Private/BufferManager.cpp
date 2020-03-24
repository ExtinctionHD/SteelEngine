#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

namespace SBufferManager
{
    vk::BufferCreateInfo GetBufferCreateInfo(const BufferDescription &description)
    {
        const QueuesDescription &queuesDescription = VulkanContext::device->GetQueuesDescription();

        const vk::BufferCreateInfo createInfo({}, description.size, description.usage,
                vk::SharingMode::eExclusive, 0, &queuesDescription.graphicsFamilyIndex);

        return createInfo;
    }
}

BufferManager::~BufferManager()
{
    if (sharedStagingBuffer.buffer)
    {
        VulkanContext::memoryManager->DestroyBuffer(sharedStagingBuffer.buffer);
    }

    for (const auto &[buffer, stagingBuffer] : buffers)
    {
        if (stagingBuffer)
        {
            VulkanContext::memoryManager->DestroyBuffer(stagingBuffer);
        }

        VulkanContext::memoryManager->DestroyBuffer(buffer->buffer);

        delete buffer;
    }
}

BufferHandle BufferManager::CreateBuffer(const BufferDescription &description, BufferCreateFlags createFlags)
{
    const vk::BufferCreateInfo createInfo = SBufferManager::GetBufferCreateInfo(description);

    Buffer *buffer = new Buffer();
    buffer->buffer = VulkanContext::memoryManager->CreateBuffer(createInfo, description.memoryProperties);
    buffer->description = description;

    vk::Buffer stagingBuffer = nullptr;
    if (createFlags & BufferCreateFlagBits::eStagingBuffer)
    {
        stagingBuffer = ResourcesHelpers::CreateStagingBuffer(description.size);
    }

    buffers.emplace(buffer, stagingBuffer);

    return buffer;
}

BufferHandle BufferManager::CreateBuffer(const BufferDescription &description, BufferCreateFlags createFlags,
        const ByteView &initialData, const SyncScope &blockedScope)
{
    Assert(initialData.size <= description.size);

    const BufferHandle handle = CreateBuffer(description, createFlags);

    if (!(createFlags & BufferCreateFlagBits::eStagingBuffer)
        && !(handle->description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible))
    {
        UpdateSharedStagingBuffer(initialData.size);

        buffers[handle] = sharedStagingBuffer.buffer;
    }

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            UpdateBuffer(commandBuffer, handle, initialData);

            const vk::BufferMemoryBarrier bufferMemoryBarrier(
                    vk::AccessFlagBits::eTransferWrite, blockedScope.access,
                    VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                    handle->buffer, 0, initialData.size);

            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, blockedScope.stages,
                    vk::DependencyFlags(), {}, { bufferMemoryBarrier }, {});
        });

    if (!(createFlags & BufferCreateFlagBits::eStagingBuffer)
        && !(handle->description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible))
    {
        buffers[handle] = nullptr;
    }

    return handle;
}

void BufferManager::DestroyBuffer(BufferHandle handle)
{
    const auto it = buffers.find(handle);
    Assert(it != buffers.end());

    const auto &[buffer, stagingBuffer] = *it;

    if (stagingBuffer)
    {
        VulkanContext::memoryManager->DestroyBuffer(stagingBuffer);
    }

    VulkanContext::memoryManager->DestroyBuffer(buffer->buffer);

    delete buffer;

    buffers.erase(it);
}

void BufferManager::UpdateBuffer(vk::CommandBuffer commandBuffer, BufferHandle handle, const ByteView &data)
{
    const auto it = buffers.find(handle);;
    Assert(it != buffers.end());

    const auto &[buffer, stagingBuffer] = *it;

    const vk::MemoryPropertyFlags memoryProperties = buffer->description.memoryProperties;
    const vk::DeviceSize bufferSize = buffer->description.size;

    if (memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)
    {
        const MemoryBlock memoryBlock = VulkanContext::memoryManager->GetBufferMemoryBlock(buffer->buffer);

        VulkanContext::memoryManager->CopyDataToMemory(data, memoryBlock);

        if (!(memoryProperties & vk::MemoryPropertyFlagBits::eHostCoherent))
        {
            const vk::MappedMemoryRange memoryRange{
                memoryBlock.memory, memoryBlock.offset, memoryBlock.size
            };

            VulkanContext::device->Get().flushMappedMemoryRanges({ memoryRange });
        }
    }
    else
    {
        Assert(commandBuffer && stagingBuffer);
        Assert(buffer->description.usage & vk::BufferUsageFlagBits::eTransferDst);

        VulkanContext::memoryManager->CopyDataToMemory(data,
                VulkanContext::memoryManager->GetBufferMemoryBlock(stagingBuffer));

        const vk::BufferCopy region(0, 0, bufferSize);

        commandBuffer.copyBuffer(stagingBuffer, buffer->buffer, { region });
    }
}
