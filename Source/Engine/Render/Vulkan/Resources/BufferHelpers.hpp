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
    void SetupPipelineBarrier(vk::CommandBuffer commandBuffer,
            vk::Buffer buffer, vk::DeviceSize size, const PipelineBarrier &barrier);

    vk::Buffer CreateUniformBuffer(vk::DeviceSize size);

    void UpdateUniformBuffer(vk::CommandBuffer commandBuffer, vk::Buffer buffer,
            const ByteView &byteView, const SyncScope &blockedScope);
}
