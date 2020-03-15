#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

bool Buffer::operator==(const Buffer &other) const
{
    return buffer == other.buffer;
}
