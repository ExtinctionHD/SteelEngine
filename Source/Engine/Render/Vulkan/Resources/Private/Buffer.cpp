#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

void Buffer::MarkForUpdate() const
{
    Assert(state != eResourceState::kUninitialized && rawData != nullptr);
    Assert(description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible
            || description.usage & vk::BufferUsageFlagBits::eTransferDst);

    state = eResourceState::kMarkedForUpdate;
}

void Buffer::FreeCpuMemory() const
{
    Assert(state != eResourceState::kUninitialized);

    delete[] rawData;

    rawData = nullptr;
}

bool Buffer::operator==(const Buffer &other) const
{
    return rawData == other.rawData && buffer == other.buffer;
}

Buffer::Buffer()
    : state(eResourceState::kUninitialized)
    , rawData(nullptr)
{}

Buffer::~Buffer()
{
    delete[] rawData;
}
