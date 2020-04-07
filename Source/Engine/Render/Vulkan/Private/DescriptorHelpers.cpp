#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

bool DescriptorDescription::operator==(const DescriptorDescription &other) const
{
    return type == other.type && stageFlags == other.stageFlags && bindingFlags == other.bindingFlags;
}
