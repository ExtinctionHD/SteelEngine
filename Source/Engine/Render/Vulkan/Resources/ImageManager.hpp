#pragma once

#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

class ImageManager
{
public:
    vk::Image CreateImage(const ImageDescription &description, ImageCreateFlags createFlags);

    vk::ImageView CreateView(vk::Image image, vk::ImageViewType viewType,
            const vk::ImageSubresourceRange &subresourceRange);

    void DestroyImage(vk::Image image);

    void UpdateImage(vk::CommandBuffer commandBuffer, vk::Image image,
            const std::vector<ImageUpdate> &imageUpdates) const;

    const ImageDescription &GetImageDescription(vk::Image image) const;

private:
    struct ImageEntry
    {
        ImageDescription description;
        vk::Buffer stagingBuffer;
        std::vector<vk::ImageView> views;
    };

    std::map<vk::Image, ImageEntry> images;
};
