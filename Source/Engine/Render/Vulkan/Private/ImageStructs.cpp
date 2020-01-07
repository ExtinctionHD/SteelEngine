#include "Engine/Render/Vulkan/ImageStructs.hpp"

bool ImageData::operator ==(const ImageData &other) const
{
    return image == other.image && view == other.view && memory == other.memory;
}