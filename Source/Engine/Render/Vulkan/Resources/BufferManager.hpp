#pragma once

#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

#include "Utils/DataHelpers.hpp"

struct SyncScope;

class BufferManager
{
public:
    vk::Buffer CreateBuffer(const BufferDescription& description, BufferCreateFlags createFlags);

    void UpdateBuffer(vk::CommandBuffer commandBuffer,
            vk::Buffer buffer, const ByteView& data) const;

    void UpdateBuffer(vk::CommandBuffer commandBuffer,
            vk::Buffer buffer, const BufferUpdater& updater) const;

    void ReadBuffer(vk::CommandBuffer commandBuffer,
            vk::Buffer buffer, const BufferReader& reader) const;

    const BufferDescription& GetBufferDescription(vk::Buffer buffer) const;

    void DestroyBuffer(vk::Buffer buffer);

private:
    struct BufferEntry
    {
        BufferDescription description;
        vk::Buffer stagingBuffer;
    };

    std::map<vk::Buffer, BufferEntry> buffers;
};
