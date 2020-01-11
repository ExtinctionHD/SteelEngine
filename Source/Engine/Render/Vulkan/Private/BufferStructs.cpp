#include "Engine/Render/Vulkan/BufferStructs.hpp"

void BufferData::MarkForUpdate()
{
    Assert(type != eBufferDataType::kUninitialized);
    type = eBufferDataType::kNeedUpdate;
}

bool BufferData::operator==(const BufferData &other) const
{
    return data == other.data && buffer == other.buffer && memory == other.memory;
}
