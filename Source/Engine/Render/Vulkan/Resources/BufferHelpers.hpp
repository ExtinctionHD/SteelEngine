#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Flags.hpp"

struct BufferDescription
{
    vk::DeviceSize size;
    vk::BufferUsageFlags usage;
    vk::MemoryPropertyFlags memoryProperties;
};

struct BufferRange
{
    vk::DeviceSize offset;
    vk::DeviceSize size;
};

enum class BufferCreateFlagBits
{
    eStagingBuffer,
};

using BufferCreateFlags = Flags<BufferCreateFlagBits>;

OVERLOAD_LOGIC_OPERATORS(BufferCreateFlags, BufferCreateFlagBits)

namespace BufferHelpers
{
    void SetupPipelineBarrier(vk::CommandBuffer commandBuffer, vk::Buffer buffer,
            const BufferRange &range, const PipelineBarrier &barrier);
}
