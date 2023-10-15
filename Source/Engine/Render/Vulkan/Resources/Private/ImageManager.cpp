#include "Engine/Render/Vulkan/Resources/ImageManager.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"

namespace Details
{
    static vk::ImageCreateFlags GetImageCreateFlags(ImageType type)
    {
        vk::ImageCreateFlags flags;

        if (type == ImageType::eCube)
        {
            flags |= vk::ImageCreateFlagBits::eCubeCompatible;
        }
        else if (type == ImageType::e3D)
        {
            flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
        }

        return flags;
    }

    static vk::ImageType GetImageType(ImageType type)
    {
        switch (type)
        {
        case ImageType::e1D:
            return vk::ImageType::e1D;
        case ImageType::e2D:
            return vk::ImageType::e2D;
        case ImageType::e3D:
            return vk::ImageType::e3D;
        case ImageType::eCube:
            return vk::ImageType::e2D;
        default:
            Assert(false);
            return {};
        }
    }

    static vk::ImageViewType GetImageViewType(ImageType type, bool isArray)
    {
        switch (type)
        {
        case ImageType::e1D:
            return isArray ? vk::ImageViewType::e1DArray : vk::ImageViewType::e1D;
        case ImageType::e2D:
            return isArray ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D;
        case ImageType::e3D:
            return vk::ImageViewType::e3D;
        case ImageType::eCube:
            return isArray ? vk::ImageViewType::eCubeArray : vk::ImageViewType::eCube;
        default:
            Assert(false);
            return {};
        }
    }

    static vk::ImageCreateInfo GetImageCreateInfo(const ImageDescription& description)
    {
        const Queues::Description& queuesDescription = VulkanContext::device->GetQueuesDescription();

        const std::vector<uint32_t> queueFamilyIndices{ queuesDescription.graphicsFamilyIndex };

        const vk::ImageCreateInfo createInfo(
                GetImageCreateFlags(description.type),
                GetImageType(description.type), description.format,
                VulkanHelpers::GetExtent3D(description.extent, description.depth),
                description.mipLevelCount, description.layerCount, description.sampleCount,
                vk::ImageTiling::eOptimal, description.usage,
                vk::SharingMode::eExclusive, queueFamilyIndices,
                vk::ImageLayout::eUndefined);

        return createInfo;
    }

    static vk::DeviceSize CalculateStagingBufferSize(const ImageDescription& description)
    {
        return ImageHelpers::CalculateMipLevelSize(description, 0) * 2;
    }

    static uint32_t CalculateDataSize(const vk::Extent3D& extent, uint32_t layerCount, vk::Format format)
    {
        return extent.width * extent.height * extent.depth * layerCount * ImageHelpers::GetTexelSize(format);
    }
}

const ImageDescription& ImageManager::GetImageDescription(vk::Image image) const
{
    return images.at(image).description;
}

vk::Image ImageManager::CreateImage(const ImageDescription& description)
{
    const vk::ImageCreateInfo createInfo = Details::GetImageCreateInfo(description);

    const vk::Image image = VulkanContext::memoryManager->CreateImage(createInfo,
            vk::MemoryPropertyFlagBits::eDeviceLocal);

    vk::Buffer stagingBuffer = nullptr;

    if (description.stagingBuffer)
    {
        stagingBuffer = BufferHelpers::CreateStagingBuffer(Details::CalculateStagingBufferSize(description));
    }

    images.emplace(image, ImageEntry{ description, {}, stagingBuffer, });

    return image;
}

BaseImage ImageManager::CreateBaseImage(const ImageDescription& description)
{
    const vk::Image image = CreateImage(description);

    const vk::ImageViewType viewType = Details::GetImageViewType(description.type, description.layerCount > 1);

    const vk::ImageSubresourceRange range(
            ImageHelpers::GetImageAspect(description.format),
            0, description.mipLevelCount,
            0, description.layerCount);

    return BaseImage{ image, CreateView(image, viewType, range) };
}

CubeImage ImageManager::CreateCubeImage(const CubeImageDescription& description)
{
    CubeImage cubeImage;

    cubeImage.image = CreateImage(description);

    const vk::ImageSubresourceRange cubeRange(
            ImageHelpers::GetImageAspect(description.format),
            0, description.mipLevelCount,
            0, ImageHelpers::kCubeFaceCount);

    cubeImage.cubeView = CreateView(cubeImage.image, vk::ImageViewType::eCube, cubeRange);

    for (uint32_t i = 0; i < ImageHelpers::kCubeFaceCount; ++i)
    {
        const vk::ImageSubresourceRange faceRange(
                ImageHelpers::GetImageAspect(description.format),
                0, description.mipLevelCount, i, 1);

        cubeImage.faceViews[i] = CreateView(cubeImage.image, vk::ImageViewType::e2D, faceRange);
    }

    return cubeImage;
}

vk::ImageView ImageManager::CreateView(vk::Image image, vk::ImageViewType viewType,
        const vk::ImageSubresourceRange& subresourceRange)
{
    auto& [description, views, stagingBuffer] = images.at(image);

    const vk::ImageViewCreateInfo createInfo({}, image, viewType,
            description.format, ImageHelpers::kComponentMappingRGBA,
            subresourceRange);

    const auto [result, view] = VulkanContext::device->Get().createImageView(createInfo);
    Assert(result == vk::Result::eSuccess);

    views.push_back(view);

    return view;
}

void ImageManager::UpdateImage(vk::CommandBuffer commandBuffer, vk::Image image,
        const std::vector<ImageUpdateRegion>& updateRegions) const
{
    const auto& [description, views, stagingBuffer] = images.at(image);

    Assert(description.usage & vk::ImageUsageFlagBits::eTransferDst);

    std::vector<vk::BufferImageCopy> copyRegions;
    copyRegions.reserve(updateRegions.size());

    MemoryBlock memoryBlock = VulkanContext::memoryManager->GetBufferMemoryBlock(stagingBuffer);

    const vk::DeviceSize stagingBufferSize = memoryBlock.size;

    vk::DeviceSize stagingBufferOffset = 0;

    for (const auto& updateRegion : updateRegions)
    {
        const uint32_t dataSize = Details::CalculateDataSize(updateRegion.extent,
                updateRegion.layers.layerCount, description.format);

        Assert(updateRegion.data.size == dataSize);
        Assert(stagingBufferOffset + updateRegion.data.size <= stagingBufferSize);

        memoryBlock.size = updateRegion.data.size;

        updateRegion.data.CopyTo(VulkanContext::memoryManager->MapMemory(memoryBlock));
        VulkanContext::memoryManager->UnmapMemory(memoryBlock);

        copyRegions.emplace_back(stagingBufferOffset, 0, 0,
                updateRegion.layers, updateRegion.offset, updateRegion.extent);

        memoryBlock.offset += updateRegion.data.size;
        stagingBufferOffset += updateRegion.data.size;
    }

    commandBuffer.copyBufferToImage(stagingBuffer, image,
            vk::ImageLayout::eTransferDstOptimal, copyRegions);
}

void ImageManager::DestroyImageView(vk::Image image, vk::ImageView view)
{
    auto& [description, views, stagingBuffer] = images.at(image);

    const auto it = std::ranges::find(views, view);
    Assert(it != views.end());

    VulkanContext::device->Get().destroyImageView(*it);

    views.erase(it);
}

void ImageManager::DestroyImage(vk::Image image)
{
    const auto& [description, views, stagingBuffer] = images.at(image);

    for (const auto& view : views)
    {
        VulkanContext::device->Get().destroyImageView(view);
    }

    if (stagingBuffer)
    {
        VulkanContext::memoryManager->DestroyBuffer(stagingBuffer);
    }

    VulkanContext::memoryManager->DestroyImage(image);

    images.erase(images.find(image));
}
