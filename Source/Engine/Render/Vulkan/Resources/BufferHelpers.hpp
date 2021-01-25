#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Flags.hpp"
#include "Utils/DataHelpers.hpp"

struct BufferDescription
{
    vk::DeviceSize size;
    vk::BufferUsageFlags usage;
    vk::MemoryPropertyFlags memoryProperties;
};

enum class BufferCreateFlagBits
{
    eStagingBuffer,
};

using BufferCreateFlags = Flags<BufferCreateFlagBits>;

OVERLOAD_LOGIC_OPERATORS(BufferCreateFlags, BufferCreateFlagBits)

namespace BufferHelpers
{
    void InsertPipelineBarrier(vk::CommandBuffer commandBuffer,
            vk::Buffer buffer, const PipelineBarrier& barrier);

    vk::Buffer CreateStagingBuffer(vk::DeviceSize size);

    void UpdateBuffer(vk::CommandBuffer commandBuffer, vk::Buffer buffer,
            const ByteView& data, const SyncScope& blockedScope);

    vk::Buffer CreateBufferWithData(vk::BufferUsageFlags bufferUsage, const ByteView& data);
}
