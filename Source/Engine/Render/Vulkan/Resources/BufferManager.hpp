#pragma once

#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

#include "Utils/DataHelpers.hpp"

struct SyncScope;

class BufferManager
{
public:
    vk::Buffer CreateBuffer(const BufferDescription& description, BufferCreateFlags createFlags);

    void DestroyBuffer(vk::Buffer buffer);

    void UpdateBuffer(vk::CommandBuffer commandBuffer, vk::Buffer buffer, const ByteView& data) const;
    
    void ReadBuffer(vk::Buffer buffer, const std::function<void(const ByteView&)>& functor) const;

    const BufferDescription& GetBufferDescription(vk::Buffer buffer) const;

private:
    struct BufferEntry
    {
        BufferDescription description;
        vk::Buffer stagingBuffer;
    };

    std::map<vk::Buffer, BufferEntry> buffers;
};
