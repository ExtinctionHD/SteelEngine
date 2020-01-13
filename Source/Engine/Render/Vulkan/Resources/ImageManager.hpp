#pragma once

#include <memory>
#include <list>

#include "Engine/Render/Vulkan/Device.hpp"

#include "Engine/Render/Vulkan/Resources/ImageData.hpp"

class ImageManager
{
public:
    static std::unique_ptr<ImageManager> Create(std::shared_ptr<Device> device);

    ImageManager(std::shared_ptr<Device> aDevice);
    ~ImageManager();

    ImageData CreateImage(const ImageProperties &properties);

    ImageData CreateView(const ImageData &imageData, const vk::ImageSubresourceRange &subresourceRange);

    ImageData CreateImageWithView(const ImageProperties& properties, const vk::ImageSubresourceRange& subresourceRange);

    ImageData Destroy(const ImageData &aImageData);

private:
    std::shared_ptr<Device> device;

    std::list<ImageData> images;
};
