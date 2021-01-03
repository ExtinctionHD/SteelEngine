#include "Engine/Render/Vulkan/Resources/ImageManager.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"

namespace Details
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

    vk::ImageCreateInfo GetImageCreateInfo(const ImageDescription& description)
    {
        const Queues::Description& queuesDescription = VulkanContext::device->GetQueuesDescription();

        const vk::ImageCreateInfo createInfo(GetVkImageCreateFlags(description.type),
                GetVkImageType(description.type), description.format, description.extent,
                description.mipLevelCount, description.layerCount, description.sampleCount,
                description.tiling, description.usage, vk::SharingMode::eExclusive, 1,
                &queuesDescription.graphicsFamilyIndex, description.initialLayout);

        return createInfo;
    }

    vk::DeviceSize CalculateStagingBufferSize(const ImageDescription& description)
    {
        return ImageHelpers::CalculateBaseMipLevelSize(description) * 2;
    }

    vk::ImageView CreateView(vk::Image image, vk::ImageViewType viewType,
            vk::Format format, const vk::ImageSubresourceRange& subresourceRange)
    {
        const vk::ImageViewCreateInfo createInfo({}, image, viewType,
                format, ImageHelpers::kComponentMappingRGBA, subresourceRange);

        const auto [result, view] = VulkanContext::device->Get().createImageView(createInfo);
        Assert(result == vk::Result::eSuccess);

        return view;
    }

    ByteView RetrieveByteView(const std::variant<Bytes, ByteView>& data)
    {
        if (std::holds_alternative<Bytes>(data))
        {
            return ByteView(std::get<Bytes>(data));
        }

        return std::get<ByteView>(data);
    }

    size_t CalculateDataSize(const vk::Extent3D& extent, uint32_t layerCount, vk::Format format)
    {
        return extent.width * extent.height * extent.depth * layerCount * ImageHelpers::GetTexelSize(format);
    }
}

vk::Image ImageManager::CreateImage(const ImageDescription& description, ImageCreateFlags createFlags)
{
    const vk::ImageCreateInfo createInfo = Details::GetImageCreateInfo(description);

    const vk::Image image = VulkanContext::memoryManager->CreateImage(createInfo, description.memoryProperties);

    vk::Buffer stagingBuffer = nullptr;
    if (createFlags & ImageCreateFlagBits::eStagingBuffer)
    {
        stagingBuffer = BufferHelpers::CreateStagingBuffer(Details::CalculateStagingBufferSize(description));
    }

    images.emplace(image, ImageEntry{ description, stagingBuffer, {} });

    return image;
}

vk::ImageView ImageManager::CreateView(vk::Image image, vk::ImageViewType viewType,
        const vk::ImageSubresourceRange& subresourceRange)
{
    auto& [description, stagingBuffer, views] = images.at(image);

    const vk::ImageView view = Details::CreateView(image, viewType, description.format, subresourceRange);

    views.push_back(view);

    return view;
}

void ImageManager::DestroyImage(vk::Image image)
{
    const auto& [description, stagingBuffer, views] = images.at(image);

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

void ImageManager::UpdateImage(vk::CommandBuffer commandBuffer, vk::Image image,
        const std::vector<ImageUpdate>& imageUpdates) const
{
    const auto& [description, stagingBuffer, views] = images.at(image);

    if (description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)
    {
        Assert(description.tiling == vk::ImageTiling::eLinear);
        Assert(false);
    }
    else
    {
        Assert(commandBuffer && images.at(image).stagingBuffer);
        Assert(description.usage & vk::ImageUsageFlagBits::eTransferDst);

        std::vector<vk::BufferImageCopy> copyRegions;
        copyRegions.reserve(imageUpdates.size());

        MemoryBlock memoryBlock = VulkanContext::memoryManager->GetBufferMemoryBlock(stagingBuffer);

        vk::DeviceSize stagingBufferOffset = 0;
        const vk::DeviceSize stagingBufferSize = memoryBlock.size;

        for (const auto& imageUpdate : imageUpdates)
        {
            const ByteView data = Details::RetrieveByteView(imageUpdate.data);

            const size_t expectedSize = Details::CalculateDataSize(imageUpdate.extent,
                    imageUpdate.layers.layerCount, description.format);

            Assert(data.size == expectedSize);
            Assert(stagingBufferOffset + data.size <= stagingBufferSize);

            memoryBlock.size = data.size;

            data.CopyTo(VulkanContext::memoryManager->MapMemory(memoryBlock));
            VulkanContext::memoryManager->UnmapMemory(memoryBlock);

            copyRegions.emplace_back(stagingBufferOffset, 0, 0,
                    imageUpdate.layers, imageUpdate.offset, imageUpdate.extent);

            memoryBlock.offset += data.size;
            stagingBufferOffset += data.size;
        }

        commandBuffer.copyBufferToImage(stagingBuffer, image,
                vk::ImageLayout::eTransferDstOptimal, copyRegions);
    }
}

const ImageDescription& ImageManager::GetImageDescription(vk::Image image) const
{
    return images.at(image).description;
}
