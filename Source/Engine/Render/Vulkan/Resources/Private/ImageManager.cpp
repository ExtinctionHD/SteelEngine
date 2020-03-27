#include "Engine/Render/Vulkan/Resources/ImageManager.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"

namespace SImageManager
{
    vk::ImageCreateFlags GetVkImageCreateFlags(ImageType type)
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

    vk::ImageType GetVkImageType(ImageType type)
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
            return vk::ImageType::e2D;
        }
    }

    vk::ImageViewType GetVkImageViewType(ImageType type, uint32_t layerCount)
    {
        switch (type)
        {
        case ImageType::e1D:
            return layerCount == 1 ? vk::ImageViewType::e1D : vk::ImageViewType::e1DArray;
        case ImageType::e2D:
            return layerCount == 1 ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray;
        case ImageType::e3D:
            return vk::ImageViewType::e3D;
        case ImageType::eCube:
            return layerCount / 6 < 2 ? vk::ImageViewType::eCube : vk::ImageViewType::eCubeArray;
        default:
            Assert(false);
            return vk::ImageViewType::e2D;
        }
    }

    vk::ImageCreateInfo GetImageCreateInfo(const ImageDescription &description)
    {
        const QueuesDescription &queuesDescription = VulkanContext::device->GetQueuesDescription();

        const vk::ImageCreateInfo createInfo(GetVkImageCreateFlags(description.type),
                GetVkImageType(description.type), description.format, description.extent,
                description.mipLevelCount, description.layerCount, description.sampleCount,
                description.tiling, description.usage, vk::SharingMode::eExclusive, 1,
                &queuesDescription.graphicsFamilyIndex, description.initialLayout);

        return createInfo;
    }

    vk::DeviceSize CalculateStagingBufferSize(const ImageDescription &description)
    {
        return description.extent.width * description.extent.width * description.extent.depth
                * ImageHelpers::GetTexelSize(description.format) * description.layerCount;
    }

    vk::ImageView CreateView(vk::Image image, const ImageDescription &description,
            const vk::ImageSubresourceRange &subresourceRange)
    {
        const vk::ImageViewCreateInfo createInfo({}, image,
                GetVkImageViewType(description.type, subresourceRange.layerCount),
                description.format, ImageHelpers::kComponentMappingRgba, subresourceRange);

        const auto [result, view] = VulkanContext::device->Get().createImageView(createInfo);
        Assert(result == vk::Result::eSuccess);

        return view;
    }

    ByteView RetrieveByteView(const std::variant<Bytes, ByteView> &data)
    {
        if (std::holds_alternative<Bytes>(data))
        {
            return GetByteView(std::get<Bytes>(data));
        }

        return std::get<ByteView>(data);
    }

    ImageLayoutTransition GetPreTransferLayoutTransition(const ImageLayoutTransition &layoutTransition)
    {
        const PipelineBarrier pipelineBarrier{
            layoutTransition.pipelineBarrier.waitedScope,
            SyncScope{
                vk::PipelineStageFlagBits::eTransfer,
                vk::AccessFlagBits::eTransferWrite
            }
        };

        return ImageLayoutTransition{
            layoutTransition.oldLayout, vk::ImageLayout::eTransferDstOptimal, pipelineBarrier
        };
    }

    ImageLayoutTransition GetPostTransferLayoutTransition(const ImageLayoutTransition &layoutTransition)
    {
        const PipelineBarrier pipelineBarrier{
            SyncScope{
                vk::PipelineStageFlagBits::eTransfer,
                vk::AccessFlagBits::eTransferWrite
            },
            layoutTransition.pipelineBarrier.blockedScope
        };

        return ImageLayoutTransition{
            vk::ImageLayout::eTransferDstOptimal, layoutTransition.newLayout, pipelineBarrier
        };
    }
}

ImageManager::~ImageManager()
{
    for (const auto &[image, entry] : images)
    {
        const auto &[description, stagingBuffer, views] = entry;

        for (auto &view : views)
        {
            VulkanContext::device->Get().destroyImageView(view);
        }

        if (stagingBuffer)
        {
            VulkanContext::memoryManager->DestroyBuffer(stagingBuffer);
        }

        VulkanContext::memoryManager->DestroyImage(image);
    }
}

vk::Image ImageManager::CreateImage(const ImageDescription &description, ImageCreateFlags createFlags)
{
    const vk::ImageCreateInfo createInfo = SImageManager::GetImageCreateInfo(description);

    const vk::Image image = VulkanContext::memoryManager->CreateImage(createInfo, description.memoryProperties);

    vk::Buffer stagingBuffer = nullptr;
    if (createFlags & ImageCreateFlagBits::eStagingBuffer)
    {
        stagingBuffer = ResourcesHelpers::CreateStagingBuffer(SImageManager::CalculateStagingBufferSize(description));
    }

    images.emplace(image, ImageEntry{ description, stagingBuffer, {} });

    return image;
}

vk::Image ImageManager::CreateImage(const ImageDescription &description, ImageCreateFlags createFlags,
        const std::vector<ImageUpdateRegion> &initialUpdateRegions)
{
    const vk::Image image = CreateImage(description, createFlags);

    if (!(createFlags & ImageCreateFlagBits::eStagingBuffer)
        && !(description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible))
    {
        UpdateSharedStagingBuffer(SImageManager::CalculateStagingBufferSize(description));

        images.at(image).stagingBuffer = sharedStagingBuffer.buffer;
    }

    VulkanContext::device->ExecuteOneTimeCommands(std::bind(&ImageManager::UpdateImage,
            this, std::placeholders::_1, image, initialUpdateRegions));

    if (!(createFlags & ImageCreateFlagBits::eStagingBuffer)
        && !(description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible))
    {
        images.at(image).stagingBuffer = nullptr;
    }

    return image;
}

vk::ImageView ImageManager::CreateView(vk::Image image, const vk::ImageSubresourceRange &subresourceRange)
{
    auto &[description, stagingBuffer, views] = images.at(image);

    const vk::ImageView view = SImageManager::CreateView(image, description, subresourceRange);

    views.push_back(view);

    return view;
}

void ImageManager::DestroyImage(vk::Image image)
{
    const auto &[description, stagingBuffer, views] = images.at(image);

    for (auto &view : views)
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

void ImageManager::UpdateImage(vk::CommandBuffer commandBuffer, vk::Image image,
        const std::vector<ImageUpdateRegion> &updateRegions) const
{
    const auto &[description, stagingBuffer, views] = images.at(image);

    if (description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)
    {
        // TODO: implement copying
        Assert(false);
    }
    else
    {
        Assert(commandBuffer && images.at(image).stagingBuffer);
        Assert(description.usage & vk::ImageUsageFlagBits::eTransferDst);

        std::vector<vk::BufferImageCopy> copyRegions;
        copyRegions.reserve(updateRegions.size());

        MemoryBlock memoryBlock = VulkanContext::memoryManager->GetBufferMemoryBlock(stagingBuffer);

        vk::DeviceSize stagingBufferOffset = 0;
        const vk::DeviceSize stagingBufferSize = memoryBlock.size;

        for (const auto &updateRegion : updateRegions)
        {
            const ByteView data = SImageManager::RetrieveByteView(updateRegion.data);
            Assert(stagingBufferOffset + data.size <= stagingBufferSize);

            memoryBlock.offset += stagingBufferOffset;
            memoryBlock.size = data.size;

            VulkanContext::memoryManager->CopyDataToMemory(data, memoryBlock);

            const vk::BufferImageCopy region(stagingBufferOffset, 0, 0,
                    ImageHelpers::GetSubresourceLayers(updateRegion.subresource),
                    updateRegion.offset, updateRegion.extent);

            copyRegions.push_back(region);

            stagingBufferOffset += data.size;

            ImageHelpers::TransitImageLayout(commandBuffer, image,
                    ImageHelpers::GetSubresourceRange(updateRegion.subresource),
                    SImageManager::GetPreTransferLayoutTransition(updateRegion.layoutTransition));
        }

        commandBuffer.copyBufferToImage(stagingBuffer, image,
                vk::ImageLayout::eTransferDstOptimal, copyRegions);

        for (const auto &updateRegion : updateRegions)
        {
            ImageHelpers::TransitImageLayout(commandBuffer, image,
                    ImageHelpers::GetSubresourceRange(updateRegion.subresource),
                    SImageManager::GetPostTransferLayoutTransition(updateRegion.layoutTransition));
        }
    }
}
