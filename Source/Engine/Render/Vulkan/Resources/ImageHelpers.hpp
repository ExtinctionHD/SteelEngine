#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
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
    vk::ImageSubresourceLayers layers;
    vk::Offset3D offset;
    vk::Extent3D extent;
    ByteView data;
};

enum class ImageCreateFlagBits
{
    eStagingBuffer
};

using ImageCreateFlags = Flags<ImageCreateFlagBits>;

OVERLOAD_LOGIC_OPERATORS(ImageCreateFlags, ImageCreateFlagBits)

namespace ImageHelpers
{
    constexpr uint32_t kCubeFaceCount = 6;

    using CubeFacesViews = std::array<vk::ImageView, kCubeFaceCount>;

    using Unorm4 = std::array<uint8_t, glm::vec4::length()>;

    constexpr std::array<glm::vec3, kCubeFaceCount> kCubeFacesDirections{
        Vector3::kX, -Vector3::kX,
        Vector3::kY, -Vector3::kY,
        Vector3::kZ, -Vector3::kZ
    };

    constexpr vk::ComponentMapping kComponentMappingRGBA(
            vk::ComponentSwizzle::eR,
            vk::ComponentSwizzle::eG,
            vk::ComponentSwizzle::eB,
            vk::ComponentSwizzle::eA);

    constexpr vk::ColorComponentFlags kColorComponentsRGBA = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    constexpr vk::ImageSubresourceRange kFlatColor(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

    constexpr vk::ImageSubresourceRange kFlatDepth(
            vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1);

    constexpr vk::ImageSubresourceRange kCubeColor(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, kCubeFaceCount);

    bool IsDepthFormat(vk::Format format);

    uint32_t GetTexelSize(vk::Format format);

    vk::ImageAspectFlags GetImageAspect(vk::Format format);

    vk::ImageSubresourceLayers GetSubresourceLayers(const vk::ImageSubresourceRange& range, uint32_t mipLevel);

    vk::ImageSubresourceRange GetSubresourceRange(const vk::ImageSubresourceLayers& layers);

    CubeFacesViews CreateCubeFacesViews(vk::Image image, uint32_t mipLevel);

    Unorm4 FloatToUnorm(const glm::vec4& value);

    uint32_t CalculateMipLevelCount(const vk::Extent2D& extent);

    uint32_t CalculateMipLevelCount(const vk::Extent3D& extent);

    vk::Extent2D CalculateMipLevelExtent(const vk::Extent2D& extent, uint32_t mipLevel);

    vk::Extent3D CalculateMipLevelExtent(const vk::Extent3D& extent, uint32_t mipLevel);

    uint32_t CalculateMipLevelTexelCount(const ImageDescription& description, uint32_t mipLevel);

    uint32_t CalculateMipLevelSize(const ImageDescription& description, uint32_t mipLevel);

    Texture CreateRenderTarget(vk::Format format, const vk::Extent2D& extent,
            vk::SampleCountFlagBits sampleCount, vk::ImageUsageFlags usage);

    void TransitImageLayout(vk::CommandBuffer commandBuffer, vk::Image image,
            const vk::ImageSubresourceRange& subresourceRange,
            const ImageLayoutTransition& layoutTransition);

    void GenerateMipLevels(vk::CommandBuffer commandBuffer, vk::Image image,
            const vk::Extent3D& extent, const vk::ImageSubresourceRange& subresourceRange);

    void ReplaceMipLevels(vk::CommandBuffer commandBuffer, vk::Image image,
            const vk::ImageSubresourceRange& subresourceRange);
}
