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

    ByteView GetData(const std::variant<Bytes, ByteView> &data)
    {
        if (std::holds_alternative<Bytes>(data))
        {
            return GetByteView(std::get<Bytes>(data));
        }

        return std::get<ByteView>(data);
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

ImageHandle ImageManager::CreateImage(const ImageDescription &description, ImageCreateFlags imageCreateFlags)
{
    const vk::ImageCreateInfo createInfo = SImageManager::GetImageCreateInfo(GetRef(device), description);

    Image *image = new Image();
    image->description = description;
    image->image = memoryManager->CreateImage(createInfo, description.memoryProperties);

    vk::Buffer stagingBuffer = nullptr;
    if (imageCreateFlags & ImageCreateFlagBits::eStagingBuffer)
    {
        stagingBuffer = ResourcesHelpers::CreateStagingBuffer(GetRef(device),
                GetRef(memoryManager), SImageManager::CalculateStagingBufferSize(description));
    }

    images.emplace(image, stagingBuffer);

    return image;
}

ImageHandle ImageManager::CreateImage(const ImageDescription &description, ImageCreateFlags imageCreateFlags,
        const std::vector<ImageUpdateRegion> &initialUpdateRegions)
{
    const ImageHandle handle = CreateImage(description, imageCreateFlags);
    for (const auto &updateRegion : initialUpdateRegions)
    {
        handle->AddUpdateRegion(updateRegion);
    }

    if (!(imageCreateFlags & ImageCreateFlagBits::eStagingBuffer)
        && !(description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible))
    {
        UpdateSharedStagingBuffer(GetRef(device), GetRef(memoryManager),
                SImageManager::CalculateStagingBufferSize(description));

        images[handle] = sharedStagingBuffer.buffer;
    }

    device->ExecuteOneTimeCommands([this, &handle](vk::CommandBuffer commandBuffer)
        {
            UpdateImage(handle, commandBuffer);
        });

    if (!(imageCreateFlags & ImageCreateFlagBits::eStagingBuffer)
        && !(description.memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible))
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

void ImageManager::UpdateImage(ImageHandle handle, vk::CommandBuffer commandBuffer) const
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
        copyRegions.reserve(image->updateRegions.size());

        MemoryBlock memoryBlock = memoryManager->GetBufferMemoryBlock(stagingBuffer);

        vk::DeviceSize stagingBufferOffset = 0;
        const vk::DeviceSize stagingBufferSize = memoryBlock.size;

        for (const auto &updateRegion : image->updateRegions)
        {
            const ByteView data = SImageManager::GetData(updateRegion.data);
            Assert(stagingBufferOffset + data.size <= stagingBufferSize);

            memoryBlock.offset += stagingBufferOffset;
            memoryBlock.size = data.size;

            memoryManager->CopyDataToMemory(data, memoryBlock);

            const vk::BufferImageCopy region(stagingBufferOffset, 0, 0,
                    VulkanHelpers::GetSubresourceLayers(updateRegion.subresource),
                    updateRegion.offset, updateRegion.extent);

            copyRegions.push_back(region);

            stagingBufferOffset += data.size;
        }

        for (const auto &updateRegion : image->updateRegions)
        {
            VulkanHelpers::UpdateImageLayout(image->image, VulkanHelpers::GetSubresourceRange(updateRegion.subresource),
                    updateRegion.oldLayout, vk::ImageLayout::eTransferDstOptimal, commandBuffer);
        }

        commandBuffer.copyBufferToImage(stagingBuffer, image->image, vk::ImageLayout::eTransferDstOptimal,
                static_cast<uint32_t>(copyRegions.size()), copyRegions.data());

        for (const auto &updateRegion : handle->updateRegions)
        {
            VulkanHelpers::UpdateImageLayout(image->image, VulkanHelpers::GetSubresourceRange(updateRegion.subresource),
                    vk::ImageLayout::eTransferDstOptimal, updateRegion.newLayout, commandBuffer);
        }

        handle->updateRegions.clear();
    }
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
