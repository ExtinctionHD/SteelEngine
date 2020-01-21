#include "Engine/Render/Vulkan/Resources/BufferDescriptor.hpp"

void BufferDescriptor::MarkForUpdate()
{
    Assert(type != eBufferDescriptorType::kUninitialized);
    Assert(properties.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible
            || properties.usage & vk::BufferUsageFlagBits::eTransferDst);

    type = eBufferDescriptorType::kNeedUpdate;
}

bool BufferDescriptor::operator==(const BufferDescriptor &other) const
{
    return data == other.data && buffer == other.buffer && memory == other.memory;
}
