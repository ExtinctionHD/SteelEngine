#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/DataHelpers.hpp"

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

struct ImageUpdateRegion
{
    std::variant<Bytes, ByteView> data;
    vk::ImageSubresource subresource;
    vk::Offset3D offset;
    vk::Extent3D extent;
    ImageLayoutTransition layoutTransition;
};

class Image
{
public:
    vk::Image image;
    std::list<vk::ImageView> views;
    ImageDescription description;

    bool operator ==(const Image &other) const;

private:
    Image() = default;
    ~Image() = default;

    friend class ImageManager;
};

using ImageHandle = const Image *;
