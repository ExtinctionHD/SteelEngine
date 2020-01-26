#include "Engine/Render/Vulkan/Resources/Image.hpp"

bool Image::operator ==(const Image &other) const
{
    return image == other.image && views == other.views;
}

Image::Image()
    : description()
    , state(eResourceState::kUninitialized)
{}
