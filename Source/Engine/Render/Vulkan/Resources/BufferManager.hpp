#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"
#include "Engine/Render/Vulkan/Resources/MemoryManager.hpp"
#include "Engine/Render/Vulkan/Resources/ResourcesHelpers.hpp"

#include "Utils/Flags.hpp"

enum class BufferCreateFlagBits
{
    eStagingBuffer,
};

using BufferCreateFlags = Flags<BufferCreateFlagBits>;

OVERLOAD_LOGIC_OPERATORS(BufferCreateFlags, BufferCreateFlagBits)

class BufferManager
        : SharedStagingBufferProvider
{
public:
    BufferManager(std::shared_ptr<Device> device_, std::shared_ptr<MemoryManager> memoryManager_);
    ~BufferManager();

    BufferHandle CreateBuffer(const BufferDescription &description, BufferCreateFlags createFlags);

    BufferHandle CreateBuffer(const BufferDescription &description, BufferCreateFlags createFlags,
            const ByteView &initialData);

    void DestroyBuffer(BufferHandle handle);

    void UpdateBuffer(vk::CommandBuffer commandBuffer, BufferHandle handle, const ByteView &data);

private:
    std::shared_ptr<Device> device;
    std::shared_ptr<MemoryManager> memoryManager;

    std::map<BufferHandle, vk::Buffer> buffers;
};
