#include "Engine/Render/Vulkan/Resources/ImageManager.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"

namespace SImageManager
{
    vk::ImageCreateFlags GetImageCreateFlags(eImageType type)
    {
        vk::ImageCreateFlags flags;

        if (type == eImageType::kCube)
        {
            flags |= vk::ImageCreateFlagBits::eCubeCompatible;
        }
        else if (type == eImageType::k3D)
        {
            flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
        }

        return flags;
    }

    vk::ImageType GetVkImageType(eImageType type)
    {
        switch (type)
        {
        case eImageType::k1D:
            return vk::ImageType::e1D;
        case eImageType::k2D:
            return vk::ImageType::e2D;
        case eImageType::k3D:
            return vk::ImageType::e3D;
        case eImageType::kCube:
            return vk::ImageType::e2D;
        default:
            Assert(false);
            return vk::ImageType::e2D;
        }
    }

    vk::ImageViewType GetVkImageViewType(eImageType type, uint32_t layerCount)
    {
        switch (type)
        {
        case eImageType::k1D:
            return layerCount == 1 ? vk::ImageViewType::e1D : vk::ImageViewType::e1DArray;
        case eImageType::k2D:
            return layerCount == 1 ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray;
        case eImageType::k3D:
            return vk::ImageViewType::e3D;
        case eImageType::kCube:
            return layerCount / 6 < 2 ? vk::ImageViewType::eCube : vk::ImageViewType::eCubeArray;
        default:
            Assert(false);
            return vk::ImageViewType::e2D;
        }
    }

    vk::Image CreateImage(const Device &device, const ImageDescription &description)
    {
        const vk::ImageCreateInfo createInfo(GetImageCreateFlags(description.type),
                GetVkImageType(description.type), description.format, description.extent,
                description.mipLevelCount, description.layerCount, description.sampleCount,
                description.tiling, description.usage, vk::SharingMode::eExclusive, 1,
                &device.GetQueueProperties().graphicsFamilyIndex, description.initialLayout);

        auto [result, image] = device.Get().createImage(createInfo);
        Assert(result == vk::Result::eSuccess);

        return image;
    }

    vk::DeviceMemory AllocateImageMemory(const Device &device, vk::Image image, vk::MemoryPropertyFlags memoryProperties)
    {
        const vk::MemoryRequirements memoryRequirements = device.Get().getImageMemoryRequirements(image);
        const vk::DeviceMemory memory = VulkanHelpers::AllocateDeviceMemory(device,
                memoryRequirements, memoryProperties);

        const vk::Result result = device.Get().bindImageMemory(image, memory, 0);
        Assert(result == vk::Result::eSuccess);

        return memory;
    }

    vk::ImageView CreateView(vk::Device device, vk::Image image, const ImageDescription &description,
            const vk::ImageSubresourceRange &subresourceRange)
    {
        const vk::ImageViewCreateInfo createInfo({}, image,
                GetVkImageViewType(description.type, subresourceRange.layerCount),
                description.format, VulkanHelpers::kComponentMappingRgba, subresourceRange);

        const auto [result, view] = device.createImageView(createInfo);
        Assert(result == vk::Result::eSuccess);

        return view;
    }

    void UpdateHostVisibleImage(vk::Device device, ImageHandle image, vk::DeviceMemory memory,
            const std::vector<ImageUpdateRegion> &updateRegions)
    {
        const vk::DeviceSize imageDataSize = device.getImageMemoryRequirements(image->image).size;
        const vk::ResultValue mappingResult = device.mapMemory(memory, 0, imageDataSize);
        Assert(mappingResult.result == vk::Result::eSuccess);

        // uint8_t *imageData = reinterpret_cast<uint8_t*>(mappingResult.value);

        for (const auto &updateRegion : updateRegions)
        {
            const uint32_t texelSize = VulkanHelpers::GetFormatTexelSize(image->description.format);
            const vk::SubresourceLayout layout = device.getImageSubresourceLayout(
                    image->image, updateRegion.subresource);

            // TODO: Implement memory copying
            Assert(false);
        }
    }
}

ImageManager::ImageManager(std::shared_ptr<Device> aDevice, std::shared_ptr<ResourceUpdateSystem> aUpdateSystem)
    : device(aDevice)
    , updateSystem(aUpdateSystem)
{}

ImageManager::~ImageManager()
{
    for (auto &[image, memory] : imageStorage)
    {
        for (auto &view : image->views)
        {
            device->Get().destroyImageView(view);
        }

        device->Get().destroyImage(image->image);
        device->Get().freeMemory(memory);
        delete image;
    }
}

ImageHandle ImageManager::CreateImage(const ImageDescription &description)
{
    Image *image = new Image();
    image->state = eResourceState::kUpdated;
    image->description = description;
    image->image = SImageManager::CreateImage(GetRef(device), description);

    const vk::DeviceMemory memory = SImageManager::AllocateImageMemory(GetRef(device),
            image->image, description.memoryProperties);

    imageStorage.emplace_back(image, memory);

    return image;
}

ImageHandle ImageManager::CreateImageWithView(const ImageDescription &description, vk::ImageAspectFlags aspectMask)
{
    const ImageHandle handle = CreateImage(description);

    const vk::ImageSubresourceRange subresourceRange{
        aspectMask, 0, description.mipLevelCount, 0, description.layerCount,
    };

    CreateView(handle, subresourceRange);

    return handle;
}

void ImageManager::CreateView(ImageHandle handle, const vk::ImageSubresourceRange &subresourceRange) const
{
    Assert(handle != nullptr && handle->state != eResourceState::kUninitialized);

    const auto it = ResourcesHelpers::FindByHandle(handle, imageStorage);
    auto &[image, memory] = *it;

    image->views.push_back(SImageManager::CreateView(device->Get(),
            image->image, image->description, subresourceRange));
}

void ImageManager::UpdateMarkedImages()
{
    for (auto &[image, memory] : imageStorage)
    {
        if (image->state == eResourceState::kMarkedForUpdate)
        {
            const vk::MemoryPropertyFlags memoryProperties = image->description.memoryProperties;
            if (memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)
            {
                Assert(image->description.tiling == vk::ImageTiling::eLinear);

                SImageManager::UpdateHostVisibleImage(device->Get(), image, memory, image->updateRegions);
            }
            else
            {
                updateSystem->EnqueueImageUpdate(image);
            }
        }
    }
}

void ImageManager::DestroyImage(ImageHandle handle)
{
    Assert(handle != nullptr && handle->state != eResourceState::kUninitialized);

    const auto it = ResourcesHelpers::FindByHandle(handle, imageStorage);
    auto &[image, memory] = *it;

    for (auto &view : image->views)
    {
        device->Get().destroyImageView(view);
    }

    device->Get().destroyImage(image->image);
    device->Get().freeMemory(memory);
    delete image;

    imageStorage.erase(it);
}
