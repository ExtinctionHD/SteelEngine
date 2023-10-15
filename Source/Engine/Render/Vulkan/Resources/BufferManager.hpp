#pragma once

#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

#include "Utils/DataHelpers.hpp"

struct SyncScope;

class BufferManager
{
public:
    vk::Buffer CreateBuffer(const BufferDescription& description);

    vk::Buffer CreateBufferWithData(vk::BufferUsageFlags usage, const ByteView& data);

    vk::Buffer CreateEmptyBuffer(vk::BufferUsageFlags usage, vk::DeviceSize size);

    const BufferDescription& GetBufferDescription(vk::Buffer buffer) const;

    void UpdateBuffer(vk::CommandBuffer commandBuffer,
            vk::Buffer buffer, const BufferUpdate& update) const;

    void ReadBuffer(vk::CommandBuffer commandBuffer,
            vk::Buffer buffer, const BufferReader& reader) const;

    void DestroyBuffer(vk::Buffer buffer);

private:
    struct BufferEntry
    {
        BufferDescription description;
        vk::Buffer stagingBuffer;
    };

    std::map<vk::Buffer, BufferEntry> buffers;
};
