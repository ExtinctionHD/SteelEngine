#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace SBufferHelpers
{
    vk::Buffer CreateDeviceLocalBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage)
    {
        const BufferDescription description{
            size, usage,
            vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible
        };

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(
                description, BufferCreateFlagBits::eStagingBuffer);

        return buffer;
    }
}

vk::DescriptorBufferInfo BufferHelpers::GetInfo(vk::Buffer buffer)
{
    return vk::DescriptorBufferInfo(buffer, 0, VK_WHOLE_SIZE);
}

void BufferHelpers::SetupPipelineBarrier(vk::CommandBuffer commandBuffer,
        vk::Buffer buffer, const PipelineBarrier &barrier)
{
    const vk::BufferMemoryBarrier bufferMemoryBarrier(
            barrier.waitedScope.access, barrier.blockedScope.access,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, buffer, 0, VK_WHOLE_SIZE);

    commandBuffer.pipelineBarrier(barrier.waitedScope.stages, barrier.blockedScope.stages,
            vk::DependencyFlags(), {}, { bufferMemoryBarrier }, {});
}

vk::Buffer BufferHelpers::CreateVertexBuffer(vk::DeviceSize size)
{
    const vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;

    return SBufferHelpers::CreateDeviceLocalBuffer(size, usage);
}

vk::Buffer BufferHelpers::CreateIndexBuffer(vk::DeviceSize size)
{
    const vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;

    return SBufferHelpers::CreateDeviceLocalBuffer(size, usage);
}

vk::Buffer BufferHelpers::CreateStorageBuffer(vk::DeviceSize size)
{
    const vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;

    return SBufferHelpers::CreateDeviceLocalBuffer(size, usage);
}

vk::Buffer BufferHelpers::CreateUniformBuffer(vk::DeviceSize size)
{
    const vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;

    return SBufferHelpers::CreateDeviceLocalBuffer(size, usage);
}

void BufferHelpers::UpdateBuffer(vk::CommandBuffer commandBuffer, vk::Buffer buffer,
        const ByteView &data, const SyncScope &blockedScope)
{
    VulkanContext::bufferManager->UpdateBuffer(commandBuffer, buffer, data);

    const PipelineBarrier barrier{
        SyncScope::kTransferWrite,
        blockedScope
    };

    BufferHelpers::SetupPipelineBarrier(commandBuffer, buffer, barrier);
}
