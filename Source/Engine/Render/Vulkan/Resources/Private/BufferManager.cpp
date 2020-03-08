#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"

#include "Utils/Helpers.hpp"

namespace SBufferManager
{
    vk::BufferCreateInfo GetBufferCreateInfo(const Device &device, const BufferDescription &description)
    {
        const vk::BufferCreateInfo createInfo({}, description.size, description.usage,
                vk::SharingMode::eExclusive, 0, &device.GetQueueProperties().graphicsFamilyIndex);

        return createInfo;
    }

    vk::Buffer CreateStagingBuffer(const Device &device, MemoryManager &memoryManager, vk::DeviceSize size)
    {
        const vk::BufferCreateInfo createInfo({}, size, vk::BufferUsageFlagBits::eTransferSrc,
                vk::SharingMode::eExclusive, 0, &device.GetQueueProperties().graphicsFamilyIndex);

        const vk::MemoryPropertyFlags memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible
                | vk::MemoryPropertyFlagBits::eHostCoherent;

        return memoryManager.CreateBuffer(createInfo, memoryProperties);
    }
}

BufferManager::BufferManager(std::shared_ptr<Device> aDevice, std::shared_ptr<MemoryManager> aMemoryManager)
    : device(aDevice)
    , memoryManager(aMemoryManager)
{}

BufferManager::~BufferManager()
{
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

BufferHandle BufferManager::CreateBuffer(const BufferDescription &description,
        const BufferAccessAbilities &accessAbilities)
{
    const vk::BufferCreateInfo createInfo = SBufferManager::GetBufferCreateInfo(GetRef(device), description);

    Buffer *buffer = new Buffer();
    buffer->description = description;
    buffer->cpuData = accessAbilities.cpuMemory ? new uint8_t[description.size] : nullptr;
    buffer->buffer = memoryManager->CreateBuffer(createInfo, description.memoryProperties);

    vk::Buffer stagingBuffer = nullptr;
    if (accessAbilities.stagingBuffer)
    {
        stagingBuffer = SBufferManager::CreateStagingBuffer(GetRef(device),
                GetRef(memoryManager), description.size);
    }

    buffers.emplace(buffer, stagingBuffer);

    return buffer;
}

void BufferManager::UpdateBuffer(BufferHandle handle, vk::CommandBuffer commandBuffer)
{
    const auto it = buffers.find(const_cast<Buffer *>(handle));;
    Assert(it != buffers.end());

    const auto &[buffer, stagingBuffer] = *it;

    const vk::MemoryPropertyFlags memoryProperties = buffer->description.memoryProperties;
    const vk::DeviceSize bufferSize = buffer->description.size;

    if (memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)
    {
        const ByteView data{ buffer->cpuData, static_cast<size_t>(bufferSize) };
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

        const ByteView data{ buffer->cpuData, static_cast<size_t>(bufferSize) };

        memoryManager->CopyDataToMemory(data, memoryManager->GetBufferMemoryBlock(stagingBuffer));

        const vk::BufferCopy region(0, 0, bufferSize);

        commandBuffer.copyBuffer(stagingBuffer, buffer->buffer, { region });
    }
}

void BufferManager::DestroyBuffer(BufferHandle handle)
{
    const auto it = buffers.find(const_cast<Buffer*>(handle));
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
