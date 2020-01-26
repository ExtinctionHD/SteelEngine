#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

void Buffer::MarkForUpdate() const
{
    Assert(state != eState::kUninitialized && data != nullptr);
    Assert(description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible
            || description.usage & vk::BufferUsageFlagBits::eTransferDst);

    state = eState::kNeedUpdate;
}

void Buffer::FreeCpuMemory() const
{
    Assert(state != eState::kUninitialized);

    delete[] data;

    data = nullptr;
}

bool Buffer::operator==(const Buffer &other) const
{
    return data == other.data && buffer == other.buffer;
}

Buffer::Buffer()
    : description()
    , state(eState::kUninitialized)
    , data(nullptr)
{}

Buffer::~Buffer()
{
    delete[] data;
}
