#pragma once

#include <memory>
#include <list>

#include "Engine/Render/Vulkan/VulkanDevice.hpp"
#include "Engine/Render/Vulkan/ImageStructs.hpp"

class ImagePool
{
public:
    static std::unique_ptr<ImagePool> Create(std::shared_ptr<VulkanDevice> device);

    ImagePool(std::shared_ptr<VulkanDevice> aDevice);
    ~ImagePool();

    ImageData CreateImage(const ImageProperties &properties);

    ImageData CreateView(const ImageData &imageData, const vk::ImageSubresourceRange &subresourceRange);

    ImageData CreateImageWithView(const ImageProperties& properties, const vk::ImageSubresourceRange& subresourceRange);

    ImageData Destroy(const ImageData &aImageData);

private:
    std::shared_ptr<VulkanDevice> device;

    std::list<ImageData> images;
};
