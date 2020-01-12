#include "Engine/Render/Vulkan/Resources/BufferData.hpp"

void BufferData::MarkForUpdate()
{
    Assert(type != eBufferDataType::kUninitialized);
    Assert(properties.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible
            || properties.usage & vk::BufferUsageFlagBits::eTransferDst);

    type = eBufferDataType::kNeedUpdate;
}

bool BufferData::operator==(const BufferData &other) const
{
    return data == other.data && buffer == other.buffer && memory == other.memory;
}
