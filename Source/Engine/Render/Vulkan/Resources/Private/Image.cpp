#include "Engine/Render/Vulkan/Resources/Image.hpp"

#include "Utils/Assert.hpp"

void Image::AddUpdateRegion(const ImageUpdateRegion &region) const
{
    Assert(description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible
            || description.usage & vk::ImageUsageFlagBits::eTransferDst);

    updateRegions.push_back(region);
}

bool Image::operator ==(const Image &other) const
{
    return image == other.image && views == other.views;
}
