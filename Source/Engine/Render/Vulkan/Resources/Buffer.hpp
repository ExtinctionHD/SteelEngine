#pragma once

#include "Engine/Render/Vulkan/Resources/ResourcesHelpers.hpp"

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
    DataAccess<T> AccessData() const;

    void MarkForUpdate() const;

    void FreeCpuMemory() const;

    bool operator ==(const Buffer &other) const;

private:
    Buffer();
    ~Buffer();

    mutable eResourceState state;
    mutable uint8_t *rawData;

    friend class BufferManager;
    friend class ResourceUpdateSystem;
};

template <class T>
DataAccess<T> Buffer::AccessData() const
{
    Assert(state != eResourceState::kUninitialized && rawData != nullptr);
    Assert(description.size % sizeof(T) == 0);

    T *typedData = reinterpret_cast<T*>(rawData);

    return { typedData, static_cast<uint32_t>(description.size / sizeof(T)) };
}

using BufferHandle = const Buffer *;
