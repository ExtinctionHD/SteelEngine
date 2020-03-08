#pragma once

#include "Utils/Assert.hpp"
#include "Utils/DataHelpers.hpp"

struct BufferDescription
{
    vk::DeviceSize size;
    vk::BufferUsageFlags usage;
    vk::MemoryPropertyFlags memoryProperties;
};

class Buffer
{
public:
    vk::Buffer buffer;
    BufferDescription description;

    template <class T>
    DataAccess<T> AccessCpuData() const;

    void FreeCpuMemory() const;

    bool operator ==(const Buffer &other) const;

private:
    Buffer();
    ~Buffer();

    mutable uint8_t *cpuData;

    friend class BufferManager;
};

template <class T>
DataAccess<T> Buffer::AccessCpuData() const
{
    Assert(cpuData != nullptr);
    Assert(description.size % sizeof(T) == 0);

    return {
        reinterpret_cast<T *>(cpuData),
        static_cast<uint32_t>(description.size / sizeof(T))
    };
}

using BufferHandle = const Buffer *;
