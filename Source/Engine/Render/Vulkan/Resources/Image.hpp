#pragma once

#include "Engine/Render/Vulkan/Resources/ResourcesHelpers.hpp"

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
    vk::ImageUsageFlagBits usage;
    vk::ImageLayout layout;

    vk::MemoryPropertyFlags memoryProperties;
};

class Image
{
public:
    vk::Image image;
    std::list<vk::ImageView> views;
    ImageDescription description;

    bool operator ==(const Image &other) const;

private:
    enum class eState
    {
        kUninitialized,
        kMarkedForUpdate,
        kUpdated
    };

    mutable eResourceState state;

    Image();

    friend class ImageManager;
};

using ImageHandle = const Image *;
