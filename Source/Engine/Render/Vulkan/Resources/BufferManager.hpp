#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"
#include "Engine/Render/Vulkan/Resources/MemoryManager.hpp"
#include "Engine/Render/Vulkan/Resources/ResourcesHelpers.hpp"

#include "Utils/Flags.hpp"

enum class BufferCreateFlagBits
{
    eCpuMemory,
    eStagingBuffer,
};

using BufferCreateFlags = Flags<BufferCreateFlagBits>;

OVERLOAD_LOGIC_OPERATORS(BufferCreateFlags, BufferCreateFlagBits)

class BufferManager
        : protected SharedStagingBufferProvider
{
public:
    BufferManager(std::shared_ptr<Device> device_, std::shared_ptr<MemoryManager> memoryManager_);
    ~BufferManager();

    BufferHandle CreateBuffer(const BufferDescription &description, BufferCreateFlags createFlags);

    template <class T>
    BufferHandle CreateBuffer(const BufferDescription &description, BufferCreateFlags createFlags,
            std::vector<T> initialData);

    void UpdateBuffer(BufferHandle handle, vk::CommandBuffer commandBuffer);

    void DestroyBuffer(BufferHandle handle);

private:
    std::shared_ptr<Device> device;
    std::shared_ptr<MemoryManager> memoryManager;

    std::map<BufferHandle, vk::Buffer> buffers;

    void SetupBufferData(BufferHandle handle, BufferCreateFlags createFlags, const ByteAccess &data);

    void RestoreBufferState(BufferHandle handle, BufferCreateFlags createFlags);
};

template <class T>
BufferHandle BufferManager::CreateBuffer(const BufferDescription &description,
        BufferCreateFlags createFlags, std::vector<T> initialData)
{
    Assert(initialData.size() * sizeof(T) <= description.size);

    const BufferHandle handle = CreateBuffer(description, createFlags);

    SetupBufferData(handle, createFlags, GetByteAccess(initialData));

    device->ExecuteOneTimeCommands([this, &handle](vk::CommandBuffer commandBuffer)
        {
            UpdateBuffer(handle, commandBuffer);
        });

    RestoreBufferState(handle, createFlags);

    return handle;
}
