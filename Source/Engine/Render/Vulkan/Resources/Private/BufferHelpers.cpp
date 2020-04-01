#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

void BufferHelpers::SetupPipelineBarrier(vk::CommandBuffer commandBuffer,
        vk::Buffer buffer, vk::DeviceSize size, const PipelineBarrier &barrier)
{
    const vk::BufferMemoryBarrier bufferMemoryBarrier(
            barrier.waitedScope.access, barrier.blockedScope.access,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, buffer, 0, size);

    commandBuffer.pipelineBarrier(barrier.waitedScope.stages, barrier.blockedScope.stages,
            vk::DependencyFlags(), {}, { bufferMemoryBarrier }, {});
}
