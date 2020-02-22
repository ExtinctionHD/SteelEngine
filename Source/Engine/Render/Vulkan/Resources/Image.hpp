#pragma once

#include "Engine/Render/Vulkan/Resources/ResourcesHelpers.hpp"

#include "Utils/DataHelpers.hpp"

enum class eImageType
{
    k1D,
    k2D,
    k3D,
    kCube,
};

struct ImageDescription
{
    eImageType type;
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
    vk::ImageLayout oldLayout;
    vk::ImageLayout newLayout;
    vk::Offset3D offset;
    vk::Extent3D extent;
};

class Image
{
public:
    vk::Image image;
    std::list<vk::ImageView> views;
    ImageDescription description;

    void MarkForUpdate(const std::vector<ImageUpdateRegion> &regions) const;

    bool operator ==(const Image &other) const;

private:
    enum class eState
    {
        kUninitialized,
        kMarkedForUpdate,
        kUpdated
    };

    mutable eResourceState state;
    mutable std::vector<ImageUpdateRegion> updateRegions;

    Image();

    friend class ImageManager;
    friend class ResourceUpdateSystem;
};

using ImageHandle = const Image *;
