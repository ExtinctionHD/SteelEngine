#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

void Buffer::MarkForUpdate() const
{
    Assert(state != eResourceState::kUninitialized && data != nullptr);
    Assert(description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible
            || description.usage & vk::BufferUsageFlagBits::eTransferDst);

    state = eResourceState::kMarkedForUpdate;
}

void Buffer::FreeCpuMemory() const
{
    Assert(state != eResourceState::kUninitialized);

    delete[] data;

    data = nullptr;
}

bool Buffer::operator==(const Buffer &other) const
{
    return data == other.data && buffer == other.buffer;
}

Buffer::Buffer()
    : state(eResourceState::kUninitialized)
    , data(nullptr)
{}

Buffer::~Buffer()
{
    delete[] data;
}
