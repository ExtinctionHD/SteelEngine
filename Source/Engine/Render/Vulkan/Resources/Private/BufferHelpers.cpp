#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

void BufferHelpers::InsertPipelineBarrier(vk::CommandBuffer commandBuffer,
        vk::Buffer buffer, const PipelineBarrier& barrier)
{
    const vk::BufferMemoryBarrier bufferMemoryBarrier(
            barrier.waitedScope.access, barrier.blockedScope.access,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, buffer, 0, VK_WHOLE_SIZE);

    commandBuffer.pipelineBarrier(barrier.waitedScope.stages, barrier.blockedScope.stages,
            vk::DependencyFlags(), {}, { bufferMemoryBarrier }, {});
}

vk::Buffer BufferHelpers::CreateStagingBuffer(vk::DeviceSize size)
{
    const Queues::Description& queuesDescription = VulkanContext::device->GetQueuesDescription();

    const vk::BufferCreateInfo createInfo({}, size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive, 0, &queuesDescription.graphicsFamilyIndex);

    const vk::MemoryPropertyFlags memoryProperties
            = vk::MemoryPropertyFlagBits::eHostVisible
            | vk::MemoryPropertyFlagBits::eHostCoherent;

    return VulkanContext::memoryManager->CreateBuffer(createInfo, memoryProperties);
}

vk::BufferUsageFlags operator|(vk::BufferUsageFlags usage, BufferType type)
{
    return usage | vk::BufferUsageFlags(static_cast<vk::BufferUsageFlags::MaskType>(type));
}

vk::BufferUsageFlags operator|(BufferType type, vk::BufferUsageFlags usage)
{
    return usage | vk::BufferUsageFlags(static_cast<vk::BufferUsageFlags::MaskType>(type));
}
