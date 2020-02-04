#include "Engine/Render/Vulkan/Resources/Image.hpp"

void Image::MarkForUpdate(const std::vector<ImageUpdateRegion> &regions) const
{
    Assert(state != eResourceState::kUninitialized);
    Assert(description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible
            || description.usage & vk::ImageUsageFlagBits::eTransferDst);

    updateRegions.insert(updateRegions.end(), regions.begin(), regions.end());
    state = eResourceState::kMarkedForUpdate;
}

bool Image::operator ==(const Image &other) const
{
    return image == other.image && views == other.views;
}

Image::Image()
    : state(eResourceState::kUninitialized)
{}
