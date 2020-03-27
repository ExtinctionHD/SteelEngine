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

struct ImageUpdateRegion
{
    std::variant<Bytes, ByteView> data;
    vk::ImageSubresource subresource;
    vk::Offset3D offset;
    vk::Extent3D extent;
    ImageLayoutTransition layoutTransition;
};

enum class ImageCreateFlagBits
{
    eStagingBuffer
};

using ImageCreateFlags = Flags<ImageCreateFlagBits>;

OVERLOAD_LOGIC_OPERATORS(ImageCreateFlags, ImageCreateFlagBits)

namespace ImageHelpers
{
    const vk::ComponentMapping kComponentMappingRgba(
            vk::ComponentSwizzle::eR,
            vk::ComponentSwizzle::eG,
            vk::ComponentSwizzle::eB,
            vk::ComponentSwizzle::eA);

    const vk::ImageSubresourceRange kSubresourceRangeFlatColor(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

    const vk::ImageSubresourceRange kSubresourceRangeFlatDepth(
            vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1);

    const vk::ColorComponentFlags kColorComponentFlagsRgba
            = vk::ColorComponentFlagBits::eR
            | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB
            | vk::ColorComponentFlagBits::eA;

    bool IsDepthFormat(vk::Format format);

    uint32_t GetTexelSize(vk::Format format);

    vk::ImageAspectFlags GetImageAspect(vk::Format format);

    vk::ImageSubresourceLayers GetSubresourceLayers(const vk::ImageSubresource &subresource);

    vk::ImageSubresourceRange GetSubresourceRange(const vk::ImageSubresource &subresource);

    void TransitImageLayout(vk::CommandBuffer commandBuffer, vk::Image image,
            const vk::ImageSubresourceRange &subresourceRange,
            const ImageLayoutTransition &layoutTransition);
}