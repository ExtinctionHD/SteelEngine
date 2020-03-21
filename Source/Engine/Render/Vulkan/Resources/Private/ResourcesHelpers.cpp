#include "Engine/Render/Vulkan/Resources/ResourcesHelpers.hpp"

void SharedStagingBufferProvider::UpdateSharedStagingBuffer(const Device &device,
        MemoryManager &memoryManager, vk::DeviceSize requiredSize)
{
    if (sharedStagingBuffer.size < requiredSize)
    {
        if (sharedStagingBuffer.buffer)
        {
            memoryManager.DestroyBuffer(sharedStagingBuffer.buffer);
        }

        sharedStagingBuffer.buffer = ResourcesHelpers::CreateStagingBuffer(device,
                memoryManager, requiredSize);

        sharedStagingBuffer.size = requiredSize;
    }
}

vk::Buffer ResourcesHelpers::CreateStagingBuffer(const Device &device, MemoryManager &memoryManager,
        vk::DeviceSize size)
{
    const vk::BufferCreateInfo createInfo({}, size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive, 0, &device.GetQueuesDescription().graphicsFamilyIndex);

    const vk::MemoryPropertyFlags memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible
            | vk::MemoryPropertyFlagBits::eHostCoherent;

    return memoryManager.CreateBuffer(createInfo, memoryProperties);
}
