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

    vk::ImageCreateInfo GetImageCreateInfo(const Device &device, const ImageDescription &description)
    {
        const vk::ImageCreateInfo createInfo(GetImageCreateFlags(description.type),
                GetVkImageType(description.type), description.format, description.extent,
                description.mipLevelCount, description.layerCount, description.sampleCount,
                description.tiling, description.usage, vk::SharingMode::eExclusive, 1,
                &device.GetQueueProperties().graphicsFamilyIndex, description.initialLayout);

        return createInfo;
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
}

ImageManager::ImageManager(std::shared_ptr<Device> aDevice, std::shared_ptr<MemoryManager> aMemoryManager,
        std::shared_ptr<ResourceUpdateSystem> aUpdateSystem)
    : device(aDevice)
    , memoryManager(aMemoryManager)
    , updateSystem(aUpdateSystem)
{}

ImageManager::~ImageManager()
{
    for (const auto &image : images)
    {
        for (auto &view : image->views)
        {
            device->Get().destroyImageView(view);
        }

        memoryManager->DestroyImage(image->image);
        delete image;
    }
}

ImageHandle ImageManager::CreateImage(const ImageDescription &description)
{
    const vk::ImageCreateInfo createInfo = SImageManager::GetImageCreateInfo(GetRef(device), description);

    Image *image = new Image();
    image->state = eResourceState::kUpdated;
    image->description = description;
    image->image = memoryManager->CreateImage(createInfo, description.memoryProperties);

    images.emplace_back(image);

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

    const auto it = std::find(images.begin(), images.end(), handle);
    Assert(it != images.end());

    Image *image = *it;

    image->views.push_back(SImageManager::CreateView(device->Get(),
            image->image, image->description, subresourceRange));
}

void ImageManager::EnqueueMarkedImagesForUpdate()
{
    for (const auto &image : images)
    {
        if (image->state == eResourceState::kMarkedForUpdate)
        {
            const vk::MemoryPropertyFlags memoryProperties = image->description.memoryProperties;
            if (memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)
            {
                Assert(image->description.tiling == vk::ImageTiling::eLinear);

                // TODO: Implement memory copying
                Assert(false);
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

    const auto it = std::find(images.begin(), images.end(), handle);
    Assert(it != images.end());

    for (auto &view : handle->views)
    {
        device->Get().destroyImageView(view);
    }

    memoryManager->DestroyImage(handle->image);
    delete handle;

    images.erase(it);
}
