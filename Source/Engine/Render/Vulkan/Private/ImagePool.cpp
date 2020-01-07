#include "Engine/Render/Vulkan/ImagePool.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"

namespace SImagePool
{
    vk::ImageType GetVkImageType(ImageProperties properties)
    {
        switch (properties.type)
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

    vk::ImageViewType GetVkImageViewType(ImageProperties properties)
    {
        switch (properties.type)
        {
        case eImageType::k1D:
            return properties.layerCount == 1 ? vk::ImageViewType::e1D : vk::ImageViewType::e1DArray;
        case eImageType::k2D:
            return properties.layerCount == 1 ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray;
        case eImageType::k3D:
            return vk::ImageViewType::e3D;
        case eImageType::kCube:
            return properties.layerCount % 6 < 2 ? vk::ImageViewType::eCube : vk::ImageViewType::eCubeArray;
        default:
            Assert(false);
            return vk::ImageViewType::e2D;
        }
    }
}

std::unique_ptr<ImagePool> ImagePool::Create(std::shared_ptr<VulkanDevice> device)
{
    return std::make_unique<ImagePool>(device);
}

ImagePool::ImagePool(std::shared_ptr<VulkanDevice> aDevice)
    : device(aDevice)
{}

ImagePool::~ImagePool()
{
    for (auto &imageData : images)
    {
        device->Get().destroyImageView(imageData.view);
        device->Get().destroyImage(imageData.image);
        device->Get().freeMemory(imageData.memory);
    }
}

ImageData ImagePool::CreateImage(const ImageProperties &properties)
{
    images.push_back(ImageData());

    ImageData &imageData = images.back();
    imageData.type = eImageDataType::kImageOnly;
    imageData.properties = properties;

    const vk::ImageCreateInfo createInfo({}, SImagePool::GetVkImageType(properties), properties.format,
            properties.extent, properties.mipLevelCount, properties.layerCount, properties.sampleCount,
            properties.tiling, properties.usage, vk::SharingMode::eExclusive, 1,
            &device->GetQueueProperties().graphicsFamilyIndex, properties.layout);

    auto [result, image] = device->Get().createImage(createInfo);
    Assert(result == vk::Result::eSuccess);
    imageData.image = image;

    const vk::MemoryRequirements memoryRequirements = device->Get().getImageMemoryRequirements(image);
    imageData.memory = VulkanHelpers::AllocateDeviceMemory(GetRef(device),
            memoryRequirements, properties.memoryProperties);

    result = device->Get().bindImageMemory(image, imageData.memory, 0);
    Assert(result == vk::Result::eSuccess);

    return imageData;
}

ImageData ImagePool::CreateView(const ImageData &aImageData,
        const vk::ImageSubresourceRange &subresourceRange)
{
    auto imageData = std::find(images.begin(), images.end(), aImageData);
    Assert(imageData != images.end());
    Assert(imageData->type == eImageDataType::kImageOnly);

    const vk::ImageViewCreateInfo createInfo({}, imageData->image,
            SImagePool::GetVkImageViewType(imageData->properties),
            imageData->properties.format, VulkanHelpers::kComponentMappingRgba,
            subresourceRange);

    const auto [result, imageView] = device->Get().createImageView(createInfo);
    Assert(result == vk::Result::eSuccess);

    imageData->view = imageView;
    imageData->type = eImageDataType::kImageWithView;

    return *imageData;
}

ImageData ImagePool::CreateImageWithView(const ImageProperties &properties,
        const vk::ImageSubresourceRange &subresourceRange)
{
    return CreateView(CreateImage(properties), subresourceRange);
}

ImageData ImagePool::Destroy(const ImageData &aImageData)
{
    Assert(aImageData.GetType() != eImageDataType::kUninitialized);

    auto imageData = std::find(images.begin(), images.end(), aImageData);
    Assert(imageData != images.end());

    device->Get().destroyImageView(imageData->view);
    device->Get().destroyImage(imageData->image);
    device->Get().freeMemory(imageData->memory);

    imageData->type = eImageDataType::kUninitialized;

    return aImageData;
}
