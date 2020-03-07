#include "Engine/Render/Vulkan/Resources/MemoryBlock.hpp"

bool MemoryBlock::operator==(const MemoryBlock &other) const
{
    return memory == other.memory && offset == other.offset && size == other.size;
}
