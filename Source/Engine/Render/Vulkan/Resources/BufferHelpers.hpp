#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/DataHelpers.hpp"

struct BufferDescription
{
    vk::DeviceSize size = 0;
    vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eStorageBuffer;
    uint32_t scratchAlignment : 1 = false;
    uint32_t stagingBuffer : 1 = false;
};

using BufferReader = std::function<void(const ByteView&)>;
using BufferUpdater = std::function<void(const ByteAccess&)>;

struct BufferUpdate
{
    ByteView data;
    SyncScope waitedScope = SyncScope::kWaitForNone;
    SyncScope blockedScope = SyncScope::kBlockNone;
    BufferUpdater updater = nullptr;
};

namespace BufferHelpers
{
    void InsertPipelineBarrier(vk::CommandBuffer commandBuffer,
            vk::Buffer buffer, const PipelineBarrier& barrier);

    vk::Buffer CreateStagingBuffer(vk::DeviceSize size);
}
