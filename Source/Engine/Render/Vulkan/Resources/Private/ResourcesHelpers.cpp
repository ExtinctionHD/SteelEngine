#include "Engine/Render/Vulkan/Resources/ResourcesHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

void SharedStagingBufferProvider::UpdateSharedStagingBuffer(vk::DeviceSize requiredSize)
{
    if (sharedStagingBuffer.size < requiredSize)
    {
        if (sharedStagingBuffer.buffer)
        {
            VulkanContext::memoryManager->DestroyBuffer(sharedStagingBuffer.buffer);
        }

        sharedStagingBuffer.buffer = ResourcesHelpers::CreateStagingBuffer(requiredSize);

        sharedStagingBuffer.size = requiredSize;
    }
}

vk::Buffer ResourcesHelpers::CreateStagingBuffer(vk::DeviceSize size)
{
    const QueuesDescription &queuesDescription = VulkanContext::device->GetQueuesDescription();

    const vk::BufferCreateInfo createInfo({}, size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive, 0, &queuesDescription.graphicsFamilyIndex);

    const vk::MemoryPropertyFlags memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible
            | vk::MemoryPropertyFlagBits::eHostCoherent;

    return VulkanContext::memoryManager->CreateBuffer(createInfo, memoryProperties);
}
