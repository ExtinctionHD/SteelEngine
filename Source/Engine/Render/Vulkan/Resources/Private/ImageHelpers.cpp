#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"

#include "Utils/Assert.hpp"

namespace Details
{
    constexpr std::array<Color, 6> kMipLevelsColors{
        Color(255, 0, 0),
        Color(255, 255, 0),
        Color(0, 255, 0),
        Color(0, 255, 255),
        Color(0, 0, 255),
        Color(255, 0, 255),
    };
}

CubeImage::operator BaseImage() const
{
    return BaseImage{ image, cubeView };
}

CubeImageDescription::operator ImageDescription() const
{
    return ImageDescription{
        .type = ImageType::eCube,
        .format = format,
        .extent = extent,
        .layerCount = ImageHelpers::kCubeFaceCount,
        .mipLevelCount = mipLevelCount,
        .sampleCount = vk::SampleCountFlagBits::e1,
        .usage = usage,
    };
}

ImageUpdateRegion2D::operator ImageUpdateRegion() const
{
    return ImageUpdateRegion{
        .layers = layers,
        .offset = vk::Offset3D(offset.x, offset.y, 0),
        .extent = vk::Extent3D(extent.width, extent.height, 1),
        .data = data,
    };
}

bool ImageHelpers::IsDepthFormat(vk::Format format)
{
    switch (format)
    {
    case vk::Format::eD16Unorm:
    case vk::Format::eX8D24UnormPack32:
    case vk::Format::eD32Sfloat:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
        return true;
    default:
        return false;
    }
}

uint32_t ImageHelpers::GetTexelSize(vk::Format format)
{
    switch (format)
    {
    case vk::Format::eUndefined:
        return 0;

    case vk::Format::eR4G4UnormPack8:
        return 1;

    case vk::Format::eR4G4B4A4UnormPack16:
    case vk::Format::eB4G4R4A4UnormPack16:
    case vk::Format::eR5G6B5UnormPack16:
    case vk::Format::eB5G6R5UnormPack16:
    case vk::Format::eR5G5B5A1UnormPack16:
    case vk::Format::eB5G5R5A1UnormPack16:
    case vk::Format::eA1R5G5B5UnormPack16:
        return 2;

    case vk::Format::eR8Unorm:
    case vk::Format::eR8Snorm:
    case vk::Format::eR8Uscaled:
    case vk::Format::eR8Sscaled:
    case vk::Format::eR8Uint:
    case vk::Format::eR8Sint:
    case vk::Format::eR8Srgb:
        return 1;

    case vk::Format::eR8G8Unorm:
    case vk::Format::eR8G8Snorm:
    case vk::Format::eR8G8Uscaled:
    case vk::Format::eR8G8Sscaled:
    case vk::Format::eR8G8Uint:
    case vk::Format::eR8G8Sint:
    case vk::Format::eR8G8Srgb:
        return 2;

    case vk::Format::eR8G8B8Unorm:
    case vk::Format::eR8G8B8Snorm:
    case vk::Format::eR8G8B8Uscaled:
    case vk::Format::eR8G8B8Sscaled:
    case vk::Format::eR8G8B8Uint:
    case vk::Format::eR8G8B8Sint:
    case vk::Format::eR8G8B8Srgb:
    case vk::Format::eB8G8R8Unorm:
    case vk::Format::eB8G8R8Snorm:
    case vk::Format::eB8G8R8Uscaled:
    case vk::Format::eB8G8R8Sscaled:
    case vk::Format::eB8G8R8Uint:
    case vk::Format::eB8G8R8Sint:
    case vk::Format::eB8G8R8Srgb:
        return 3;

    case vk::Format::eR8G8B8A8Unorm:
    case vk::Format::eR8G8B8A8Snorm:
    case vk::Format::eR8G8B8A8Uscaled:
    case vk::Format::eR8G8B8A8Sscaled:
    case vk::Format::eR8G8B8A8Uint:
    case vk::Format::eR8G8B8A8Sint:
    case vk::Format::eR8G8B8A8Srgb:
    case vk::Format::eB8G8R8A8Unorm:
    case vk::Format::eB8G8R8A8Snorm:
    case vk::Format::eB8G8R8A8Uscaled:
    case vk::Format::eB8G8R8A8Sscaled:
    case vk::Format::eB8G8R8A8Uint:
    case vk::Format::eB8G8R8A8Sint:
    case vk::Format::eB8G8R8A8Srgb:
    case vk::Format::eA8B8G8R8UnormPack32:
    case vk::Format::eA8B8G8R8SnormPack32:
    case vk::Format::eA8B8G8R8UscaledPack32:
    case vk::Format::eA8B8G8R8SscaledPack32:
    case vk::Format::eA8B8G8R8UintPack32:
    case vk::Format::eA8B8G8R8SintPack32:
    case vk::Format::eA8B8G8R8SrgbPack32:
        return 4;

    case vk::Format::eA2R10G10B10UnormPack32:
    case vk::Format::eA2R10G10B10SnormPack32:
    case vk::Format::eA2R10G10B10UscaledPack32:
    case vk::Format::eA2R10G10B10SscaledPack32:
    case vk::Format::eA2R10G10B10UintPack32:
    case vk::Format::eA2R10G10B10SintPack32:
    case vk::Format::eA2B10G10R10UnormPack32:
    case vk::Format::eA2B10G10R10SnormPack32:
    case vk::Format::eA2B10G10R10UscaledPack32:
    case vk::Format::eA2B10G10R10SscaledPack32:
    case vk::Format::eA2B10G10R10UintPack32:
    case vk::Format::eA2B10G10R10SintPack32:
        return 4;

    case vk::Format::eR16Unorm:
    case vk::Format::eR16Snorm:
    case vk::Format::eR16Uscaled:
    case vk::Format::eR16Sscaled:
    case vk::Format::eR16Uint:
    case vk::Format::eR16Sint:
    case vk::Format::eR16Sfloat:
        return 2;

    case vk::Format::eR16G16Unorm:
    case vk::Format::eR16G16Snorm:
    case vk::Format::eR16G16Uscaled:
    case vk::Format::eR16G16Sscaled:
    case vk::Format::eR16G16Uint:
    case vk::Format::eR16G16Sint:
    case vk::Format::eR16G16Sfloat:
        return 4;

    case vk::Format::eR16G16B16Unorm:
    case vk::Format::eR16G16B16Snorm:
    case vk::Format::eR16G16B16Uscaled:
    case vk::Format::eR16G16B16Sscaled:
    case vk::Format::eR16G16B16Uint:
    case vk::Format::eR16G16B16Sint:
    case vk::Format::eR16G16B16Sfloat:
        return 6;

    case vk::Format::eR16G16B16A16Unorm:
    case vk::Format::eR16G16B16A16Snorm:
    case vk::Format::eR16G16B16A16Uscaled:
    case vk::Format::eR16G16B16A16Sscaled:
    case vk::Format::eR16G16B16A16Uint:
    case vk::Format::eR16G16B16A16Sint:
    case vk::Format::eR16G16B16A16Sfloat:
        return 8;

    case vk::Format::eR32Uint:
    case vk::Format::eR32Sint:
    case vk::Format::eR32Sfloat:
        return 4;

    case vk::Format::eR32G32Uint:
    case vk::Format::eR32G32Sint:
    case vk::Format::eR32G32Sfloat:
        return 8;

    case vk::Format::eR32G32B32Uint:
    case vk::Format::eR32G32B32Sint:
    case vk::Format::eR32G32B32Sfloat:
        return 12;

    case vk::Format::eR32G32B32A32Uint:
    case vk::Format::eR32G32B32A32Sint:
    case vk::Format::eR32G32B32A32Sfloat:
        return 16;

    case vk::Format::eR64Uint:
    case vk::Format::eR64Sint:
    case vk::Format::eR64Sfloat:
        return 8;

    case vk::Format::eR64G64Uint:
    case vk::Format::eR64G64Sint:
    case vk::Format::eR64G64Sfloat:
        return 16;

    case vk::Format::eR64G64B64Uint:
    case vk::Format::eR64G64B64Sint:
    case vk::Format::eR64G64B64Sfloat:
        return 24;

    case vk::Format::eR64G64B64A64Uint:
    case vk::Format::eR64G64B64A64Sint:
    case vk::Format::eR64G64B64A64Sfloat:
        return 32;

    case vk::Format::eB10G11R11UfloatPack32:
    case vk::Format::eE5B9G9R9UfloatPack32:
        return 4;

    case vk::Format::eD16Unorm:
        return 2;

    case vk::Format::eX8D24UnormPack32:
        return 4;

    case vk::Format::eD32Sfloat:
        return 4;

    case vk::Format::eS8Uint:
        return 1;

    case vk::Format::eD16UnormS8Uint:
        return 3;

    case vk::Format::eD24UnormS8Uint:
        return 4;

    case vk::Format::eD32SfloatS8Uint:
        return 5;

    default:
        Assert(false);
        return 0;
    }
}

vk::ImageAspectFlags ImageHelpers::GetImageAspect(vk::Format format)
{
    switch (format)
    {
    case vk::Format::eD32Sfloat:
    case vk::Format::eD16Unorm:
        return vk::ImageAspectFlagBits::eDepth;

    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
        return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

    default:
        return vk::ImageAspectFlagBits::eColor;
    }
}

vk::ImageSubresourceRange ImageHelpers::GetSubresourceRange(const ImageDescription& description)
{
    return vk::ImageSubresourceRange(
            GetImageAspect(description.format),
            0, description.mipLevelCount,
            0, description.layerCount);
}

vk::ImageSubresourceRange ImageHelpers::GetSubresourceRange(const vk::ImageSubresourceLayers& layers)
{
    return vk::ImageSubresourceRange(layers.aspectMask, layers.mipLevel, 1, layers.baseArrayLayer, layers.layerCount);
}

vk::ImageSubresourceLayers ImageHelpers::GetSubresourceLayers(const ImageDescription& description, uint32_t mipLevel)
{
    return vk::ImageSubresourceLayers(
            GetImageAspect(description.format),
            mipLevel, 0, description.layerCount);
}

uint32_t ImageHelpers::CalculateMipLevelCount(const vk::Extent2D& extent)
{
    const float maxSize = static_cast<float>(std::max(extent.width, extent.height));
    return 1 + static_cast<uint32_t>(std::floorf(std::log2f(maxSize)));
}

uint32_t ImageHelpers::CalculateMipLevelCount(const vk::Extent3D& extent)
{
    const float maxSize = static_cast<float>(std::max(extent.width, std::max(extent.height, extent.depth)));
    return 1 + static_cast<uint32_t>(std::floorf(std::log2f(maxSize)));
}

vk::Extent2D ImageHelpers::CalculateMipLevelExtent(const vk::Extent2D& extent, uint32_t mipLevel)
{
    return vk::Extent2D(
            std::max(static_cast<uint32_t>(extent.width * std::pow(0.5f, mipLevel)), 1u),
            std::max(static_cast<uint32_t>(extent.height * std::pow(0.5f, mipLevel)), 1u));
}

vk::Extent3D ImageHelpers::CalculateMipLevelExtent(const vk::Extent3D& extent, uint32_t mipLevel)
{
    return vk::Extent3D(
            std::max(static_cast<uint32_t>(extent.width * std::pow(0.5f, mipLevel)), 1u),
            std::max(static_cast<uint32_t>(extent.height * std::pow(0.5f, mipLevel)), 1u),
            std::max(static_cast<uint32_t>(extent.depth * std::pow(0.5f, mipLevel)), 1u));
}

uint32_t ImageHelpers::CalculateMipLevelTexelCount(const ImageDescription& description, uint32_t mipLevel)
{
    const vk::Extent3D extent = VulkanHelpers::GetExtent3D(description.extent, description.depth);

    const vk::Extent3D mipLevelExtent = CalculateMipLevelExtent(extent, mipLevel);

    return mipLevelExtent.width * mipLevelExtent.height * mipLevelExtent.depth * description.layerCount;
}

uint32_t ImageHelpers::CalculateMipLevelSize(const ImageDescription& description, uint32_t mipLevel)
{
    return CalculateMipLevelTexelCount(description, mipLevel) * GetTexelSize(description.format);
}

void ImageHelpers::TransitImageLayout(vk::CommandBuffer commandBuffer, vk::Image image,
        const vk::ImageSubresourceRange& subresourceRange,
        const ImageLayoutTransition& layoutTransition)
{
    const auto& [oldLayout, newLayout, pipelineBarrier] = layoutTransition;

    const vk::ImageMemoryBarrier imageMemoryBarrier(
            pipelineBarrier.waitedScope.access,
            pipelineBarrier.blockedScope.access,
            oldLayout, newLayout,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            image, subresourceRange);

    commandBuffer.pipelineBarrier(
            pipelineBarrier.waitedScope.stages,
            pipelineBarrier.blockedScope.stages,
            vk::DependencyFlags(), {}, {},
            { imageMemoryBarrier });
}

void ImageHelpers::GenerateMipLevels(vk::CommandBuffer commandBuffer, vk::Image image,
        vk::ImageLayout initialLayout, vk::ImageLayout finalLayout)
{
    const ImageDescription& description = ResourceContext::GetImageDescription(image);

    {
        vk::ImageSubresourceRange baseMipLevelRange = GetSubresourceRange(description);
        baseMipLevelRange.levelCount = 1;

        const ImageLayoutTransition layoutTransition{
            initialLayout,
            vk::ImageLayout::eTransferSrcOptimal,
            PipelineBarrier{
                SyncScope::kWaitForAll,
                SyncScope::kTransferRead
            }
        };

        TransitImageLayout(commandBuffer, image, baseMipLevelRange, layoutTransition);
    }

    {
        vk::ImageSubresourceRange mipLevelsRange = GetSubresourceRange(description);
        mipLevelsRange.levelCount = mipLevelsRange.levelCount - 1;
        mipLevelsRange.baseMipLevel = 1;

        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            PipelineBarrier{
                SyncScope::kWaitForNone,
                SyncScope::kTransferWrite
            }
        };

        TransitImageLayout(commandBuffer, image, mipLevelsRange, layoutTransition);
    }

    vk::Offset3D srcExtent(
            static_cast<int32_t>(description.extent.width),
            static_cast<int32_t>(description.extent.height),
            static_cast<int32_t>(description.depth));

    for (uint32_t mipLevel = 0; mipLevel < description.mipLevelCount - 1; ++mipLevel)
    {
        const vk::ImageSubresourceLayers srcLayers = GetSubresourceLayers(description, mipLevel);

        const vk::ImageSubresourceLayers dstLayers = GetSubresourceLayers(description, mipLevel + 1);


        const vk::Offset3D dstExtent(
                std::max(srcExtent.x / 2, 1),
                std::max(srcExtent.y / 2, 1),
                std::max(srcExtent.z / 2, 1));

        const vk::ImageBlit region(
                srcLayers, { vk::Offset3D(0, 0, 0), srcExtent },
                dstLayers, { vk::Offset3D(0, 0, 0), dstExtent });

        commandBuffer.blitImage(
                image, vk::ImageLayout::eTransferSrcOptimal,
                image, vk::ImageLayout::eTransferDstOptimal,
                { region }, vk::Filter::eLinear);

        {
            const ImageLayoutTransition layoutTransition{
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eTransferSrcOptimal,
                PipelineBarrier{
                    SyncScope::kTransferRead,
                    SyncScope::kTransferWrite
                }
            };

            TransitImageLayout(commandBuffer, image, GetSubresourceRange(dstLayers), layoutTransition);
        }

        srcExtent = dstExtent;
    }

    {
        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eTransferSrcOptimal,
            finalLayout,
            PipelineBarrier{
                SyncScope::kTransferRead,
                SyncScope::kBlockAll
            }
        };

        TransitImageLayout(commandBuffer, image, GetSubresourceRange(description), layoutTransition);
    }
}

void ImageHelpers::ReplaceMipLevelsWithColors(vk::CommandBuffer commandBuffer, vk::Image image)
{
    const ImageDescription& description = ResourceContext::GetImageDescription(image);

    const vk::Extent3D extent = VulkanHelpers::GetExtent3D(description.extent, description.depth);

    std::vector<Bytes> colorData(description.mipLevelCount);
    std::vector<ImageUpdateRegion> updateRegions(description.mipLevelCount);

    for (uint32_t mipLevel = 0; mipLevel < description.mipLevelCount; ++mipLevel)
    {
        const uint32_t colorIndex = mipLevel % static_cast<uint32_t>(Details::kMipLevelsColors.size());

        const Color& color = Details::kMipLevelsColors[colorIndex];

        const uint32_t texelCount = CalculateMipLevelTexelCount(description, mipLevel);

        switch (description.format)
        {
        case vk::Format::eR32G32B32A32Sfloat:
            colorData[mipLevel] = CopyVector<LinearColor, uint8_t>(Repeat(LinearColor(color), texelCount));
            break;

        case vk::Format::eR8G8B8A8Unorm:
            colorData[mipLevel] = CopyVector<Color, uint8_t>(Repeat(color, texelCount));
            break;

        default:
            Assert(false);
            break;
        }

        Assert(colorData[mipLevel].size() == CalculateMipLevelSize(description, mipLevel));

        const ImageUpdateRegion imageUpdateRegion{
            GetSubresourceLayers(description, mipLevel),
            vk::Offset3D(0, 0, 0),
            CalculateMipLevelExtent(extent, mipLevel),
            ByteView(colorData[mipLevel])
        };

        updateRegions[mipLevel] = imageUpdateRegion;
    }

    ResourceContext::UpdateImage(commandBuffer, image, updateRegions);
}
