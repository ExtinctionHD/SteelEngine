#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

struct BaseImage
{
    vk::Image image;
    vk::ImageView view;
};

using RenderTarget = BaseImage;

using CubeFaceViews = std::array<vk::ImageView, 6>;

enum class ImageType
{
    e1D,
    e2D,
    e3D,
    eCube,
};

struct ImageDescription
{
    ImageType type = ImageType::e2D;
    vk::Format format = vk::Format::eR8G8B8A8Unorm;
    vk::Extent2D extent = vk::Extent2D(1, 1);

    uint32_t depth = 1;
    uint32_t layerCount = 1;
    uint32_t mipLevelCount = 1;
    vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage;
    uint32_t stagingBuffer : 1 = false;
};

struct CubeImageDescription
{
    vk::Format format = vk::Format::eR8G8B8A8Unorm;
    vk::Extent2D extent = vk::Extent2D(1, 1);

    uint32_t mipLevelCount = 1;
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled;

    operator ImageDescription() const;
};

struct ImageViewDescription
{
    vk::Image image;
    vk::ImageViewType viewType;
    vk::ImageSubresourceRange subresourceRange;
};

struct ImageLayoutTransition
{
    vk::ImageLayout oldLayout;
    vk::ImageLayout newLayout;
    PipelineBarrier pipelineBarrier;
};

struct ImageUpdateRegion
{
    vk::ImageSubresourceLayers layers;
    vk::Offset3D offset;
    vk::Extent3D extent;
    ByteView data;
};

struct ImageUpdateRegion2D
{
    vk::ImageSubresourceLayers layers;
    vk::Offset2D offset;
    vk::Extent2D extent;
    ByteView data;

    operator ImageUpdateRegion() const;
};

using ImageUpdateRegions = std::vector<ImageUpdateRegion>;

using Unorm4 = std::array<uint8_t, 4>;

namespace ImageHelpers
{
    constexpr uint32_t kCubeFaceCount = 6;

    constexpr std::array<glm::vec3, 6> kCubeFacesDirections{
        Vector3::kX, -Vector3::kX,
        Vector3::kY, -Vector3::kY,
        Vector3::kZ, -Vector3::kZ
    };

    constexpr vk::ComponentMapping kComponentMappingRGBA(
            vk::ComponentSwizzle::eR,
            vk::ComponentSwizzle::eG,
            vk::ComponentSwizzle::eB,
            vk::ComponentSwizzle::eA);

    constexpr vk::ColorComponentFlags kColorComponentsRGBA
            = vk::ColorComponentFlagBits::eR
            | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB
            | vk::ColorComponentFlagBits::eA;

    constexpr vk::ImageSubresourceRange kFlatColor(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

    constexpr vk::ImageSubresourceRange kFlatDepth(
            vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1);

    constexpr vk::ImageSubresourceRange kCubeColor(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, kCubeFaceCount);

    bool IsDepthFormat(vk::Format format);

    uint32_t GetTexelSize(vk::Format format);

    vk::ImageAspectFlags GetImageAspect(vk::Format format);

    vk::ImageSubresourceRange GetSubresourceRange(const ImageDescription& description);

    vk::ImageSubresourceRange GetSubresourceRange(const vk::ImageSubresourceLayers& layers);

    vk::ImageSubresourceLayers GetSubresourceLayers(const ImageDescription& description, uint32_t mipLevel);

    uint32_t CalculateMipLevelCount(const vk::Extent2D& extent);

    uint32_t CalculateMipLevelCount(const vk::Extent3D& extent);

    vk::Extent2D CalculateMipLevelExtent(const vk::Extent2D& extent, uint32_t mipLevel);

    vk::Extent3D CalculateMipLevelExtent(const vk::Extent3D& extent, uint32_t mipLevel);

    uint32_t CalculateMipLevelTexelCount(const ImageDescription& description, uint32_t mipLevel);

    uint32_t CalculateMipLevelSize(const ImageDescription& description, uint32_t mipLevel);

    void TransitImageLayout(vk::CommandBuffer commandBuffer, vk::Image image,
            const vk::ImageSubresourceRange& subresourceRange,
            const ImageLayoutTransition& layoutTransition);

    void GenerateMipLevels(vk::CommandBuffer commandBuffer, vk::Image image,
            vk::ImageLayout initialLayout, vk::ImageLayout finalLayout);

    void ReplaceMipLevelsWithColors(vk::CommandBuffer commandBuffer, vk::Image image);
}
