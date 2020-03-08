#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

void Buffer::FreeCpuMemory() const
{
    delete[] cpuData;

    cpuData = nullptr;
}

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
