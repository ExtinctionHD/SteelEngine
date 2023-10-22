#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/DataHelpers.hpp"

enum class BufferType
{
    eNone = 0,
    eUniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    eStorage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    eIndex = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    eVertex = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
};

struct BufferDescription
{
    BufferType type = BufferType::eNone;
    vk::DeviceSize size = 0;
    vk::BufferUsageFlags usage;
    uint32_t scratchAlignment : 1 = false;
    uint32_t stagingBuffer : 1 = false;
    ByteView initialData;
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

vk::BufferUsageFlags operator|(vk::BufferUsageFlags usage, BufferType type);
vk::BufferUsageFlags operator|(BufferType type, vk::BufferUsageFlags usage);
