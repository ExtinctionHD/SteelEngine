#pragma once

#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

class ImageManager
{
public:
    const ImageDescription& GetImageDescription(vk::Image image) const;

    vk::Image CreateImage(const ImageDescription& description);

    BaseImage CreateBaseImage(const ImageDescription& description);

    CubeImage CreateCubeImage(const CubeImageDescription& description);

    vk::ImageView CreateView(vk::Image image, vk::ImageViewType viewType,
            const vk::ImageSubresourceRange& subresourceRange);

    void UpdateImage(vk::CommandBuffer commandBuffer, vk::Image image,
            const std::vector<ImageUpdateRegion>& updateRegions) const;

    void DestroyImageView(vk::Image image, vk::ImageView view);

    void DestroyImage(vk::Image image);

private:
    struct ImageEntry
    {
        ImageDescription description;
        std::vector<vk::ImageView> views;
        vk::Buffer stagingBuffer;
    };

    std::map<vk::Image, ImageEntry> images;
};
