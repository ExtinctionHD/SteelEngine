#include "Engine/Render/Vulkan/Resources/BufferDescriptor.hpp"

void BufferDescriptor::MarkForUpdate()
{
    Assert(type != eBufferDescriptorType::kUninitialized);
    Assert(description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible
            || description.usage & vk::BufferUsageFlagBits::eTransferDst);

    type = eBufferDescriptorType::kNeedUpdate;
}

bool BufferDescriptor::operator==(const BufferDescriptor &other) const
{
    return data == other.data && buffer == other.buffer && memory == other.memory;
}
