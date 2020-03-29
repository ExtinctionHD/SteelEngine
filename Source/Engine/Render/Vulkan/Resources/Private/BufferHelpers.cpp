#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

void BufferHelpers::SetupPipelineBarrier(vk::CommandBuffer commandBuffer, vk::Buffer buffer,
        const BufferRange &range, const PipelineBarrier &barrier)
{
    const vk::BufferMemoryBarrier bufferMemoryBarrier(
            barrier.waitedScope.access, barrier.blockedScope.access,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            buffer, range.offset, range.size);

    commandBuffer.pipelineBarrier(barrier.waitedScope.stages, barrier.blockedScope.stages,
            vk::DependencyFlags(), {}, { bufferMemoryBarrier }, {});
}
