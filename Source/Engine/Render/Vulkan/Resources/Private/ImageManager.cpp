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
}

ImageManager::ImageManager(std::shared_ptr<Device> aDevice, std::shared_ptr<TransferSystem> aTransferSystem)
    : device(aDevice)
    , transferSystem(aTransferSystem)
{}

ImageManager::~ImageManager()
{
    for (auto &imageDescriptor : images)
    {
        if (imageDescriptor.type != eImageDescriptorType::kUninitialized)
        {
            device->Get().destroyImageView(imageDescriptor.view);
            device->Get().destroyImage(imageDescriptor.image);
            device->Get().freeMemory(imageDescriptor.memory);
        }
    }
}

ImageDescriptor ImageManager::CreateImage(const ImageDescription &description)
{
    images.push_back(ImageDescriptor());

    ImageDescriptor &imageDescriptor = images.back();
    imageDescriptor.type = eImageDescriptorType::kImageOnly;
    imageDescriptor.description = description;

    const vk::ImageCreateInfo createInfo(SImageManager::GetImageCreateFlags(description.type),
            SImageManager::GetVkImageType(description.type), description.format, description.extent,
            description.mipLevelCount, description.layerCount, description.sampleCount,
            description.tiling, description.usage, vk::SharingMode::eExclusive, 1,
            &device->GetQueueProperties().graphicsFamilyIndex, description.layout);

    auto [result, image] = device->Get().createImage(createInfo);
    Assert(result == vk::Result::eSuccess);
    imageDescriptor.image = image;

    const vk::MemoryRequirements memoryRequirements = device->Get().getImageMemoryRequirements(image);
    imageDescriptor.memory = VulkanHelpers::AllocateDeviceMemory(GetRef(device),
            memoryRequirements, description.memoryProperties);

    result = device->Get().bindImageMemory(image, imageDescriptor.memory, 0);
    Assert(result == vk::Result::eSuccess);

    return imageDescriptor;
}

ImageDescriptor ImageManager::CreateView(const ImageDescriptor &aImageDescriptor,
        const vk::ImageSubresourceRange &subresourceRange)
{
    auto imageDescriptor = std::find(images.begin(), images.end(), aImageDescriptor);
    Assert(imageDescriptor != images.end());
    Assert(imageDescriptor->type == eImageDescriptorType::kImageOnly);

    const vk::ImageViewCreateInfo createInfo({}, imageDescriptor->image,
            SImageManager::GetVkImageViewType(imageDescriptor->description.type, subresourceRange.layerCount),
            imageDescriptor->description.format, VulkanHelpers::kComponentMappingRgba,
            subresourceRange);

    const auto [result, imageView] = device->Get().createImageView(createInfo);
    Assert(result == vk::Result::eSuccess);

    imageDescriptor->view = imageView;
    imageDescriptor->type = eImageDescriptorType::kImageWithView;

    return *imageDescriptor;
}

ImageDescriptor ImageManager::CreateImageWithView(const ImageDescription &description,
        const vk::ImageSubresourceRange &subresourceRange)
{
    return CreateView(CreateImage(description), subresourceRange);
}

ImageDescriptor ImageManager::Destroy(const ImageDescriptor &aImageDescriptor)
{
    Assert(aImageDescriptor.GetType() != eImageDescriptorType::kUninitialized);

    auto imageDescriptor = std::find(images.begin(), images.end(), aImageDescriptor);
    Assert(imageDescriptor != images.end());

    device->Get().destroyImageView(imageDescriptor->view);
    device->Get().destroyImage(imageDescriptor->image);
    device->Get().freeMemory(imageDescriptor->memory);

    imageDescriptor->type = eImageDescriptorType::kUninitialized;

    return *imageDescriptor;
}
