#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Helpers.hpp"

namespace SBufferManager
{
    vk::BufferCreateInfo GetBufferCreateInfo(const Device &device, const BufferDescription &description)
    {
        const vk::BufferCreateInfo createInfo({}, description.size, description.usage,
                vk::SharingMode::eExclusive, 0, &device.GetQueuesDescription().graphicsFamilyIndex);

        return createInfo;
    }
}

BufferManager::BufferManager(std::shared_ptr<Device> device_, std::shared_ptr<MemoryManager> memoryManager_)
    : device(device_)
    , memoryManager(memoryManager_)
{}

BufferManager::~BufferManager()
{
    if (sharedStagingBuffer.buffer)
    {
        memoryManager->DestroyBuffer(sharedStagingBuffer.buffer);
    }

    for (const auto &[buffer, stagingBuffer] : buffers)
    {
        if (stagingBuffer)
        {
            memoryManager->DestroyBuffer(stagingBuffer);
        }

        memoryManager->DestroyBuffer(buffer->buffer);

        delete buffer;
    }
}

BufferHandle BufferManager::CreateBuffer(const BufferDescription &description, BufferCreateFlags createFlags)
{
    const vk::BufferCreateInfo createInfo = SBufferManager::GetBufferCreateInfo(GetRef(device), description);

    Buffer *buffer = new Buffer();
    buffer->buffer = memoryManager->CreateBuffer(createInfo, description.memoryProperties);
    buffer->description = description;

    vk::Buffer stagingBuffer = nullptr;
    if (createFlags & BufferCreateFlagBits::eStagingBuffer)
    {
        stagingBuffer = ResourcesHelpers::CreateStagingBuffer(GetRef(device),
                GetRef(memoryManager), description.size);
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
        UpdateSharedStagingBuffer(GetRef(device), GetRef(memoryManager), initialData.size);

        buffers[handle] = sharedStagingBuffer.buffer;
    }

    device->ExecuteOneTimeCommands([this, &handle, &initialData, blockedScope](vk::CommandBuffer commandBuffer)
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
        memoryManager->DestroyBuffer(stagingBuffer);
    }

    memoryManager->DestroyBuffer(buffer->buffer);

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
        const MemoryBlock memoryBlock = memoryManager->GetBufferMemoryBlock(buffer->buffer);

        memoryManager->CopyDataToMemory(data, memoryBlock);

        if (!(memoryProperties & vk::MemoryPropertyFlagBits::eHostCoherent))
        {
            const vk::MappedMemoryRange memoryRange{
                memoryBlock.memory, memoryBlock.offset, memoryBlock.size
            };

            device->Get().flushMappedMemoryRanges({ memoryRange });
        }
    }
    else
    {
        Assert(commandBuffer && stagingBuffer);
        Assert(buffer->description.usage & vk::BufferUsageFlagBits::eTransferDst);

        memoryManager->CopyDataToMemory(data, memoryManager->GetBufferMemoryBlock(stagingBuffer));

        const vk::BufferCopy region(0, 0, bufferSize);

        commandBuffer.copyBuffer(stagingBuffer, buffer->buffer, { region });
    }
}
