#include "Engine/Render/Vulkan/Resources/ImageDescriptor.hpp"

bool ImageDescriptor::operator ==(const ImageDescriptor &other) const
{
    return image == other.image && view == other.view && memory == other.memory;
}
