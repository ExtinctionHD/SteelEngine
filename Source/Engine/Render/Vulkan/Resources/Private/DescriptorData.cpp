#include "Engine/Render/Vulkan/Resources/DescriptorData.hpp"

bool DescriptorProperties::operator==(const DescriptorProperties &other) const
{
    return type == other.type && stageFlags == other.stageFlags;
}
