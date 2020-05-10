#include "Engine/Render/Vulkan/Resources/ResourcesHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

vk::Buffer ResourcesHelpers::CreateStagingBuffer(vk::DeviceSize size)
{
    const Queues::Description &queuesDescription = VulkanContext::device->GetQueuesDescription();

    const vk::BufferCreateInfo createInfo({}, size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive, 0, &queuesDescription.graphicsFamilyIndex);

    const vk::MemoryPropertyFlags memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible
            | vk::MemoryPropertyFlagBits::eHostCoherent;

    return VulkanContext::memoryManager->CreateBuffer(createInfo, memoryProperties);
}
