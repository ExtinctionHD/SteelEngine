#pragma once

#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

#include "Utils/DataHelpers.hpp"

struct SyncScope;

class BufferManager
{
public:
    BufferManager() = default;
    ~BufferManager();

    vk::Buffer CreateBuffer(const BufferDescription &description, BufferCreateFlags createFlags);

    void DestroyBuffer(vk::Buffer buffer);

    void UpdateBuffer(vk::CommandBuffer commandBuffer, vk::Buffer buffer, const ByteView &data);

private:
    struct BufferEntry
    {
        BufferDescription description;
        vk::Buffer stagingBuffer;
    };

    std::map<vk::Buffer, BufferEntry> buffers;
};
