#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

bool Buffer::operator==(const Buffer &other) const
{
    return cpuData == other.cpuData && buffer == other.buffer;
}

Buffer::Buffer()
    : cpuData(nullptr)
{}

Buffer::~Buffer()
{
    delete[] cpuData;
}
