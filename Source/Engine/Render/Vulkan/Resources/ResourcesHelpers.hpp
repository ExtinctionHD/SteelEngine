#pragma once

class SharedStagingBufferProvider
{
protected:
    struct SharedStagingBuffer
    {
        vk::Buffer buffer = vk::Buffer();
        vk::DeviceSize size = 0;
    };

    SharedStagingBuffer sharedStagingBuffer;

    void UpdateSharedStagingBuffer(vk::DeviceSize requiredSize);
};

namespace ResourcesHelpers
{
    vk::Buffer CreateStagingBuffer(vk::DeviceSize size);
}
