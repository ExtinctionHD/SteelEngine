#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Render/Vulkan/Resources/MemoryManager.hpp"

class ImageManager
{
public:
    ImageManager(std::shared_ptr<Device> aDevice, std::shared_ptr<MemoryManager> aMemoryManager);
    ~ImageManager();

    ImageHandle CreateImage(const ImageDescription &description, vk::DeviceSize stagingBufferSize);

    void CreateView(ImageHandle handle, const vk::ImageSubresourceRange &subresourceRange) const;

    void UpdateImage(ImageHandle handle, vk::CommandBuffer commandBuffer) const;

    void DestroyImage(ImageHandle handle);

private:
    std::shared_ptr<Device> device;
    std::shared_ptr<MemoryManager> memoryManager;

    std::map<Image *, vk::Buffer> images;
};
