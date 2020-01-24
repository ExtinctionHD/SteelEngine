#pragma once

#include <memory>
#include <list>

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/ImageDescriptor.hpp"
#include "Engine/Render/Vulkan/Resources/TransferSystem.hpp"

class ImageManager
{
public:
    ImageManager(std::shared_ptr<Device> aDevice, std::shared_ptr<TransferSystem> aTransferSystem);
    ~ImageManager();

    ImageDescriptor CreateImage(const ImageProperties &properties);

    ImageDescriptor CreateView(const ImageDescriptor &imageDescriptor,
            const vk::ImageSubresourceRange &subresourceRange);

    ImageDescriptor CreateImageWithView(const ImageProperties &properties,
            const vk::ImageSubresourceRange &subresourceRange);

    ImageDescriptor Destroy(const ImageDescriptor &aImageDescriptor);

private:
    std::shared_ptr<Device> device;
    std::shared_ptr<TransferSystem> transferSystem;

    std::list<ImageDescriptor> images;
};
