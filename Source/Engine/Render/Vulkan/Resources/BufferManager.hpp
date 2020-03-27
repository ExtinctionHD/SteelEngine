#pragma once

#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/ResourcesHelpers.hpp"

#include "Utils/DataHelpers.hpp"

struct SyncScope;

class BufferManager
        : SharedStagingBufferProvider
{
public:
    BufferManager() = default;
    ~BufferManager() override;

    vk::Buffer CreateBuffer(const BufferDescription &description, BufferCreateFlags createFlags);

    vk::Buffer CreateBuffer(const BufferDescription &description, BufferCreateFlags createFlags,
            const ByteView &initialData, const SyncScope &blockedScope);

    void DestroyBuffer(vk::Buffer handle);

    void UpdateBuffer(vk::CommandBuffer commandBuffer, vk::Buffer handle, const ByteView &data);

private:
    struct BufferEntry
    {
        BufferDescription description;
        vk::Buffer stagingBuffer;
    };

    std::map<vk::Buffer, BufferEntry> buffers;
};
