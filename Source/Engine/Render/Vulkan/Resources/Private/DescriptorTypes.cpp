#include "Engine/Render/Vulkan/Resources/DescriptorTypes.hpp"

bool DescriptorProperties::operator==(const DescriptorProperties &other) const
{
    return type == other.type && stageFlags == other.stageFlags;
}
