#pragma once

#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/ResourcesHelpers.hpp"

class ImageManager
        : SharedStagingBufferProvider
{
public:
    ImageManager() = default;
    ~ImageManager() override;

    vk::Image CreateImage(const ImageDescription &description, ImageCreateFlags createFlags);

    vk::Image CreateImage(const ImageDescription &description, ImageCreateFlags createFlags,
            const std::vector<ImageUpdateRegion> &initialUpdateRegions);

    vk::ImageView CreateView(vk::Image image, const vk::ImageSubresourceRange &subresourceRange);

    void DestroyImage(vk::Image image);

    void UpdateImage(vk::CommandBuffer commandBuffer, vk::Image image,
            const std::vector<ImageUpdateRegion> &updateRegions) const;

private:
    struct ImageEntry
    {
        ImageDescription description;
        vk::Buffer stagingBuffer;
        std::vector<vk::ImageView> views;
    };

    std::map<vk::Image, ImageEntry> images;
};
