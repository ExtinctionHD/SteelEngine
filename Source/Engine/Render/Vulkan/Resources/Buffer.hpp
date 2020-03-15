#pragma once

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

    bool operator ==(const Buffer &other) const;

private:
    Buffer() = default;
    ~Buffer() = default;

    friend class BufferManager;
};

using BufferHandle = const Buffer *;
