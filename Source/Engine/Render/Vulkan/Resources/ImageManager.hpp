#pragma once

#include <memory>
#include <list>

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Render/Vulkan/Resources/TransferSystem.hpp"

class ImageManager
{
public:
    ImageManager(std::shared_ptr<Device> aDevice, std::shared_ptr<TransferSystem> aTransferSystem);
    ~ImageManager();

    ImageHandle CreateImage(const ImageDescription &description);

    ImageHandle CreateImageWithView(const ImageDescription &description, vk::ImageAspectFlags aspectMask);

    void CreateView(ImageHandle handle, const vk::ImageSubresourceRange &subresourceRange) const;

    void Destroy(ImageHandle handle);

private:
    std::shared_ptr<Device> device;
    std::shared_ptr<TransferSystem> transferSystem;

    ResourceStorage<Image> images;
};
