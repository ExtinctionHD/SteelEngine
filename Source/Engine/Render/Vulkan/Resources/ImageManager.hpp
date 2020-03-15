#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Render/Vulkan/Resources/MemoryManager.hpp"
#include "Engine/Render/Vulkan/Resources/ResourcesHelpers.hpp"

#include "Utils/Flags.hpp"

enum class ImageCreateFlagBits
{
    eStagingBuffer
};

using ImageCreateFlags = Flags<ImageCreateFlagBits>;

OVERLOAD_LOGIC_OPERATORS(ImageCreateFlags, ImageCreateFlagBits)

class ImageManager
        : protected SharedStagingBufferProvider
{
public:
    ImageManager(std::shared_ptr<Device> device_, std::shared_ptr<MemoryManager> memoryManager_);
    ~ImageManager();

    ImageHandle CreateImage(const ImageDescription &description, ImageCreateFlags createFlags);

    ImageHandle CreateImage(const ImageDescription &description, ImageCreateFlags createFlags,
            const std::vector<ImageUpdateRegion> &initialUpdateRegions);

    void CreateView(ImageHandle handle, const vk::ImageSubresourceRange &subresourceRange) const;

    void DestroyImage(ImageHandle handle);

    void UpdateImage(vk::CommandBuffer commandBuffer, ImageHandle handle,
            const std::vector<ImageUpdateRegion> &updateRegions) const;

private:
    std::shared_ptr<Device> device;
    std::shared_ptr<MemoryManager> memoryManager;

    std::map<ImageHandle, vk::Buffer> images;
};
