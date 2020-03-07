#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceUpdateSystem.hpp"
#include "Engine/Render/Vulkan/Resources/MemoryManager.hpp"

class ImageManager
{
public:
    ImageManager(std::shared_ptr<Device> aDevice, std::shared_ptr<MemoryManager> aMemoryManager,
            std::shared_ptr<ResourceUpdateSystem> aUpdateSystem);
    ~ImageManager();

    ImageHandle CreateImage(const ImageDescription &description);

    ImageHandle CreateImageWithView(const ImageDescription &description, vk::ImageAspectFlags aspectMask);

    void CreateView(ImageHandle handle, const vk::ImageSubresourceRange &subresourceRange) const;

    void EnqueueMarkedImagesForUpdate();

    void DestroyImage(ImageHandle handle);

private:
    std::shared_ptr<Device> device;
    std::shared_ptr<MemoryManager> memoryManager;
    std::shared_ptr<ResourceUpdateSystem> updateSystem;

    std::list<Image*> images;
};
