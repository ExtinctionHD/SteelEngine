#pragma once

#include "Utils/Assert.hpp"

enum class eBufferDescriptorType
{
    kUninitialized,
    kNeedUpdate,
    kValid
};

struct BufferDescription
{
    vk::DeviceSize size;
    vk::BufferUsageFlags usage;
    vk::MemoryPropertyFlags memoryProperties;
};

class BufferDescriptor
{
public:
    template <class T>
    struct AccessStruct
    {
        T *data;
        uint32_t count;
    };

    eBufferDescriptorType GetType() const { return type; }

    const BufferDescription &GetDescription() const { return description; }

    vk::Buffer GetBuffer() const { return buffer; }

    template <class T>
    AccessStruct<T> AccessData() const;

    void MarkForUpdate();

    bool operator ==(const BufferDescriptor &other) const;

private:
    eBufferDescriptorType type = eBufferDescriptorType::kUninitialized;
    BufferDescription description = {};

    uint8_t *data = nullptr;

    vk::Buffer buffer;
    vk::DeviceMemory memory;

    friend class BufferManager;
};

template <class T>
BufferDescriptor::AccessStruct<T> BufferDescriptor::AccessData() const
{
    Assert(type != eBufferDescriptorType::kUninitialized);
    Assert(description.size % sizeof(T) == 0);

    T *typedData = reinterpret_cast<T*>(data);

    return { typedData, static_cast<uint32_t>(description.size / sizeof(T)) };
}
