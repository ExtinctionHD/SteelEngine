#pragma once

#include "Utils/Assert.hpp"

struct BufferDescription
{
    vk::DeviceSize size;
    vk::BufferUsageFlags usage;
    vk::MemoryPropertyFlags memoryProperties;
};

class Buffer
{
public:
    template <class T>
    struct AccessStruct
    {
        T *data;
        uint32_t count;
    };

    vk::Buffer buffer;
    BufferDescription description;

    template <class T>
    AccessStruct<T> AccessData() const;

    void MarkForUpdate() const;

    void FreeCpuMemory() const;

    bool operator ==(const Buffer &other) const;

private:
    enum class eState
    {
        kUninitialized,
        kNeedUpdate,
        kUpdated
    };

    Buffer();
    ~Buffer();

    mutable eState state;
    mutable uint8_t *data;

    friend class BufferManager;
    friend class TransferSystem;
};

template <class T>
Buffer::AccessStruct<T> Buffer::AccessData() const
{
    Assert(state != eState::kUninitialized || data != nullptr);
    Assert(description.size % sizeof(T) == 0);

    T *typedData = reinterpret_cast<T*>(data);

    return { typedData, static_cast<uint32_t>(description.size / sizeof(T)) };
}

using BufferHandle = const Buffer *;
