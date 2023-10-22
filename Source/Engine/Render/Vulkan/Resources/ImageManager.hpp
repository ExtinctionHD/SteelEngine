#pragma once

#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

class ImageManager
{
public:
    const ImageDescription& GetImageDescription(vk::Image image) const;

    vk::Image CreateImage(const ImageDescription& description);

    BaseImage CreateBaseImage(const ImageDescription& description);

    CubeImage CreateCubeImage(const CubeImageDescription& description);

    vk::ImageView CreateView(const ImageViewDescription& description);

    void UpdateImage(vk::CommandBuffer commandBuffer,
            vk::Image image, const ImageUpdateRegions& updateRegions) const;

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
