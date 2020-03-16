#include "Engine/Render/Vulkan/Resources/ImageManager.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"

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

    vk::ImageCreateInfo GetImageCreateInfo(const Device &device, const ImageDescription &description)
    {
        const vk::ImageCreateInfo createInfo(GetVkImageCreateFlags(description.type),
                GetVkImageType(description.type), description.format, description.extent,
                description.mipLevelCount, description.layerCount, description.sampleCount,
                description.tiling, description.usage, vk::SharingMode::eExclusive, 1,
                &device.GetQueueProperties().graphicsFamilyIndex, description.initialLayout);

        return createInfo;
    }

    vk::DeviceSize CalculateStagingBufferSize(const ImageDescription &description)
    {
        return description.extent.width * description.extent.width * description.extent.depth
                * VulkanHelpers::GetFormatTexelSize(description.format) * description.layerCount;
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

    ByteView RetrieveByteView(const std::variant<Bytes, ByteView> &data)
    {
        if (std::holds_alternative<Bytes>(data))
        {
            return GetByteView(std::get<Bytes>(data));
        }

        return std::get<ByteView>(data);
    }

    ImageLayoutTransition GetPreTransferTransition(const ImageLayoutTransition &transition)
    {
        const PipelineBarrier pipelineBarrier{
            transition.pipelineBarrier.waitedScope,
            SynchronizationScope{
                vk::PipelineStageFlagBits::eTransfer,
                vk::AccessFlagBits::eTransferWrite
            }
        };

        return ImageLayoutTransition{ transition.oldLayout, vk::ImageLayout::eTransferDstOptimal, pipelineBarrier };
    }

    ImageLayoutTransition GetPostTransferTransition(const ImageLayoutTransition &transition)
    {
        const PipelineBarrier pipelineBarrier{
            SynchronizationScope{
                vk::PipelineStageFlagBits::eTransfer,
                vk::AccessFlagBits::eTransferWrite
            },
            transition.pipelineBarrier.blockedScope
        };

        return ImageLayoutTransition{ vk::ImageLayout::eTransferDstOptimal, transition.newLayout, pipelineBarrier };
    }
}

ImageManager::ImageManager(std::shared_ptr<Device> device_, std::shared_ptr<MemoryManager> memoryManager_)
    : device(device_)
    , memoryManager(memoryManager_)
{}

ImageManager::~ImageManager()
{
    for (const auto &[image, stagingBuffer] : images)
    {
        for (auto &view : image->views)
        {
            device->Get().destroyImageView(view);
        }

        if (stagingBuffer)
        {
            memoryManager->DestroyBuffer(stagingBuffer);
        }

        memoryManager->DestroyImage(image->image);

        delete image;
    }
}

ImageHandle ImageManager::CreateImage(const ImageDescription &description, ImageCreateFlags createFlags)
{
    const vk::ImageCreateInfo createInfo = SImageManager::GetImageCreateInfo(GetRef(device), description);

    Image *image = new Image();
    image->description = description;
    image->image = memoryManager->CreateImage(createInfo, description.memoryProperties);

    vk::Buffer stagingBuffer = nullptr;
    if (createFlags & ImageCreateFlagBits::eStagingBuffer)
    {
        stagingBuffer = ResourcesHelpers::CreateStagingBuffer(GetRef(device),
                GetRef(memoryManager), SImageManager::CalculateStagingBufferSize(description));
    }

    images.emplace(image, stagingBuffer);

    return image;
}

ImageHandle ImageManager::CreateImage(const ImageDescription &description, ImageCreateFlags createFlags,
        const std::vector<ImageUpdateRegion> &initialUpdateRegions)
{
    const ImageHandle handle = CreateImage(description, createFlags);

    if (!(createFlags & ImageCreateFlagBits::eStagingBuffer)
        && !(handle->description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible))
    {
        UpdateSharedStagingBuffer(GetRef(device), GetRef(memoryManager),
                SImageManager::CalculateStagingBufferSize(handle->description));

        images[handle] = sharedStagingBuffer.buffer;
    }

    device->ExecuteOneTimeCommands(std::bind(&ImageManager::UpdateImage,
            this, std::placeholders::_1, handle, initialUpdateRegions));

    if (!(createFlags & ImageCreateFlagBits::eStagingBuffer)
        && !(handle->description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible))
    {
        images[handle] = nullptr;
    }

    return handle;
}

void ImageManager::CreateView(ImageHandle handle, const vk::ImageSubresourceRange &subresourceRange) const
{
    const auto it = images.find(handle);
    Assert(it != images.end());

    Image *image = const_cast<Image *>(it->first);

    const vk::ImageView view = SImageManager::CreateView(device->Get(),
            image->image, image->description, subresourceRange);

    image->views.push_back(view);
}

void ImageManager::DestroyImage(ImageHandle handle)
{
    const auto it = images.find(handle);;
    Assert(it != images.end());

    const auto &[image, stagingBuffer] = *it;

    for (auto &view : image->views)
    {
        device->Get().destroyImageView(view);
    }

    if (stagingBuffer)
    {
        memoryManager->DestroyBuffer(stagingBuffer);
    }

    memoryManager->DestroyImage(image->image);

    delete image;

    images.erase(it);
}

void ImageManager::UpdateImage(vk::CommandBuffer commandBuffer, ImageHandle handle,
        const std::vector<ImageUpdateRegion> &updateRegions) const
{
    const auto it = images.find(handle);
    Assert(it != images.end());

    const auto &[image, stagingBuffer] = *it;

    if (image->description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)
    {
        // TODO: implement copying
        Assert(false);
    }
    else
    {
        Assert(commandBuffer && stagingBuffer);
        Assert(image->description.usage & vk::ImageUsageFlagBits::eTransferDst);

        std::vector<vk::BufferImageCopy> copyRegions;
        copyRegions.reserve(updateRegions.size());

        MemoryBlock memoryBlock = memoryManager->GetBufferMemoryBlock(stagingBuffer);

        vk::DeviceSize stagingBufferOffset = 0;
        const vk::DeviceSize stagingBufferSize = memoryBlock.size;

        for (const auto &updateRegion : updateRegions)
        {
            const ByteView data = SImageManager::RetrieveByteView(updateRegion.data);
            Assert(stagingBufferOffset + data.size <= stagingBufferSize);

            memoryBlock.offset += stagingBufferOffset;
            memoryBlock.size = data.size;

            memoryManager->CopyDataToMemory(data, memoryBlock);

            const vk::BufferImageCopy region(stagingBufferOffset, 0, 0,
                    VulkanHelpers::GetSubresourceLayers(updateRegion.subresource),
                    updateRegion.offset, updateRegion.extent);

            copyRegions.push_back(region);

            stagingBufferOffset += data.size;

            VulkanHelpers::TransitImageLayout(commandBuffer, image->image,
                    VulkanHelpers::GetSubresourceRange(updateRegion.subresource),
                    SImageManager::GetPreTransferTransition(updateRegion.layoutTransition));
        }

        commandBuffer.copyBufferToImage(stagingBuffer, image->image,
                vk::ImageLayout::eTransferDstOptimal, copyRegions);

        for (const auto &updateRegion : updateRegions)
        {
            VulkanHelpers::TransitImageLayout(commandBuffer, image->image,
                    VulkanHelpers::GetSubresourceRange(updateRegion.subresource),
                    SImageManager::GetPostTransferTransition(updateRegion.layoutTransition));
        }
    }
}
