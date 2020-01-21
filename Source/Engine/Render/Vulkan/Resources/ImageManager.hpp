#pragma once

#include <memory>
#include <list>

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/ImageDescriptor.hpp"

class ImageManager
{
public:
    static std::unique_ptr<ImageManager> Create(std::shared_ptr<Device> device);

    ImageManager(std::shared_ptr<Device> aDevice);
    ~ImageManager();

    ImageDescriptor CreateImage(const ImageProperties &properties);

    ImageDescriptor CreateView(const ImageDescriptor &imageDescriptor, const vk::ImageSubresourceRange &subresourceRange);

    ImageDescriptor CreateImageWithView(const ImageProperties& properties, const vk::ImageSubresourceRange& subresourceRange);

    ImageDescriptor Destroy(const ImageDescriptor &aImageDescriptor);

private:
    std::shared_ptr<Device> device;

    std::list<ImageDescriptor> images;
};
