#pragma once

#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Render/Vulkan/Resources/ResourcesHelpers.hpp"

#include "Utils/Flags.hpp"

enum class ImageCreateFlagBits
{
    eStagingBuffer
};

using ImageCreateFlags = Flags<ImageCreateFlagBits>;

OVERLOAD_LOGIC_OPERATORS(ImageCreateFlags, ImageCreateFlagBits)

class ImageManager
        : protected SharedStagingBufferProvider
{
public:
    ImageManager() = default;
    ~ImageManager();

    ImageHandle CreateImage(const ImageDescription &description, ImageCreateFlags createFlags);

    ImageHandle CreateImage(const ImageDescription &description, ImageCreateFlags createFlags,
            const std::vector<ImageUpdateRegion> &initialUpdateRegions);

    void CreateView(ImageHandle handle, const vk::ImageSubresourceRange &subresourceRange) const;

    void DestroyImage(ImageHandle handle);

    void UpdateImage(vk::CommandBuffer commandBuffer, ImageHandle handle,
            const std::vector<ImageUpdateRegion> &updateRegions) const;

private:
    std::map<ImageHandle, vk::Buffer> images;
};
