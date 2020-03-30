#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/DataHelpers.hpp"
#include "Utils/Flags.hpp"

enum class ImageType
{
    e1D,
    e2D,
    e3D,
    eCube,
};

struct ImageDescription
{
    ImageType type;
    vk::Format format;
    vk::Extent3D extent;

    uint32_t mipLevelCount;
    uint32_t layerCount;
    vk::SampleCountFlagBits sampleCount;

    vk::ImageTiling tiling;
    vk::ImageUsageFlags usage;
    vk::ImageLayout initialLayout;

    vk::MemoryPropertyFlags memoryProperties;
};

struct ImageLayoutTransition
{
    vk::ImageLayout oldLayout;
    vk::ImageLayout newLayout;
    PipelineBarrier pipelineBarrier;
};

struct ImageUpdate
{
    ByteView data;
    vk::ImageSubresourceLayers layers;
    vk::Offset3D offset;
    vk::Extent3D extent;
};

enum class ImageCreateFlagBits
{
    eStagingBuffer
};

using ImageCreateFlags = Flags<ImageCreateFlagBits>;

OVERLOAD_LOGIC_OPERATORS(ImageCreateFlags, ImageCreateFlagBits)

namespace ImageHelpers
{
    const vk::ComponentMapping kComponentMappingRGBA(
            vk::ComponentSwizzle::eR,
            vk::ComponentSwizzle::eG,
            vk::ComponentSwizzle::eB,
            vk::ComponentSwizzle::eA);

    const vk::ColorComponentFlags kColorComponentFlagsRGBA
            = vk::ColorComponentFlagBits::eR
            | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB
            | vk::ColorComponentFlagBits::eA;

    const vk::ImageSubresourceRange kFlatColor(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

    const vk::ImageSubresourceRange kFlatDepth(
            vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1);

    bool IsDepthFormat(vk::Format format);

    uint32_t GetTexelSize(vk::Format format);

    vk::DeviceSize CalculateBaseMipLevelSize(const ImageDescription &description);

    vk::ImageAspectFlags GetImageAspect(vk::Format format);

    vk::ImageSubresourceLayers GetSubresourceLayers(const vk::ImageSubresourceRange &range, uint32_t mipLevel);

    vk::ImageSubresourceRange GetSubresourceRange(const vk::ImageSubresourceLayers &layers);

    void TransitImageLayout(vk::CommandBuffer commandBuffer, vk::Image image,
            const vk::ImageSubresourceRange &subresourceRange,
            const ImageLayoutTransition &layoutTransition);

    void GenerateMipmaps(vk::CommandBuffer commandBuffer, vk::Image image,
            const vk::Extent3D &extent, const vk::ImageSubresourceRange &subresourceRange);
}
