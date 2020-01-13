#pragma once

#include "Utils/Assert.hpp"

enum class eBufferDataType
{
    kUninitialized,
    kNeedUpdate,
    kValid
};

struct BufferProperties
{
    uint32_t size;
    vk::BufferUsageFlags usage;

    vk::MemoryPropertyFlags memoryProperties;
};

class BufferData
{
public:
    template <class T>
    struct AccessStruct
    {
        T *data;
        uint32_t count;
    };

    const eBufferDataType &GetType() const { return type; }

    const BufferProperties &GetProperties() const { return properties; }

    vk::Buffer GetBuffer() const { return buffer; }

    template <class T>
    AccessStruct<T> AccessData() const;

    void MarkForUpdate();

    bool operator ==(const BufferData &other) const;

private:
    BufferData() = default;

    eBufferDataType type = eBufferDataType::kUninitialized;
    BufferProperties properties = {};

    uint8_t *data = nullptr;

    vk::Buffer buffer;
    vk::DeviceMemory memory;

    friend class BufferManager;
};

template <class T>
BufferData::AccessStruct<T> BufferData::AccessData() const
{
    Assert(type != eBufferDataType::kUninitialized);
    Assert(properties.size % sizeof(T) == 0);

    T *typedData = reinterpret_cast<T*>(data);

    return { typedData, properties.size / sizeof(T) };
}
