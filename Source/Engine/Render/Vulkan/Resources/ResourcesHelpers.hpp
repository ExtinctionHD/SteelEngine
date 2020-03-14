#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/MemoryManager.hpp"

class SharedStagingBufferProvider
{
protected:
    struct SharedStagingBuffer
    {
        vk::Buffer buffer = vk::Buffer();
        vk::DeviceSize size = 0;
    };

    SharedStagingBuffer sharedStagingBuffer;

    void UpdateSharedStagingBuffer(const Device &device,
            MemoryManager &memoryManager, vk::DeviceSize requiredSize);
};

namespace ResourcesHelpers
{
    vk::Buffer CreateStagingBuffer(const Device &device, MemoryManager &memoryManager, vk::DeviceSize size);
}
