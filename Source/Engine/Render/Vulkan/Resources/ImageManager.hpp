#pragma once

#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

class ImageManager
{
public:
    const ImageDescription& GetImageDescription(vk::Image image) const;

    vk::Image CreateImage(const ImageDescription& description);

    BaseImage CreateBaseImage(const ImageDescription& description);

    BaseImage CreateCubeImage(const CubeImageDescription& description);

    vk::ImageView CreateView(const ImageViewDescription& description);

    CubeFaceViews CreateCubeFaceViews(vk::Image image);

    void UpdateImage(vk::CommandBuffer commandBuffer,
            vk::Image image, const ImageUpdateRegions& updateRegions) const;

    void DestroyImage(vk::Image image);

    void DestroyView(vk::ImageView view);

private:
    struct ImageEntry
    {
        ImageDescription description;
        std::set<vk::ImageView> views;
        vk::Buffer stagingBuffer;
    };

    std::map<vk::Image, ImageEntry> images;
    std::map<vk::ImageView, vk::Image> viewMap;
};
