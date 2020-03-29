#pragma once

#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

class ImageManager
{
public:
    ImageManager() = default;
    ~ImageManager();

    vk::Image CreateImage(const ImageDescription &description, ImageCreateFlags createFlags);

    vk::ImageView CreateView(vk::Image image, const vk::ImageSubresourceRange &subresourceRange);

    void DestroyImage(vk::Image image);

    void UpdateImage(vk::CommandBuffer commandBuffer, vk::Image image,
            const std::vector<ImageUpdate> &imageUpdates) const;

private:
    struct ImageEntry
    {
        ImageDescription description;
        vk::Buffer stagingBuffer;
        std::vector<vk::ImageView> views;
    };

    std::map<vk::Image, ImageEntry> images;
};
