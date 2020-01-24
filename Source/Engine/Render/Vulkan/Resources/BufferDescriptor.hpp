#pragma once

#include "Utils/Assert.hpp"

enum class eBufferDescriptorType
{
    kUninitialized,
    kNeedUpdate,
    kValid
};

struct BufferProperties
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

    const BufferProperties &GetProperties() const { return properties; }

    vk::Buffer GetBuffer() const { return buffer; }

    template <class T>
    AccessStruct<T> AccessData() const;

    void MarkForUpdate();

    bool operator ==(const BufferDescriptor &other) const;

private:
    eBufferDescriptorType type = eBufferDescriptorType::kUninitialized;
    BufferProperties properties = {};

    uint8_t *data = nullptr;

    vk::Buffer buffer;
    vk::DeviceMemory memory;

    friend class BufferManager;
};

template <class T>
BufferDescriptor::AccessStruct<T> BufferDescriptor::AccessData() const
{
    Assert(type != eBufferDescriptorType::kUninitialized);
    Assert(properties.size % sizeof(T) == 0);

    T *typedData = reinterpret_cast<T*>(data);

    return { typedData, static_cast<uint32_t>(properties.size / sizeof(T)) };
}
