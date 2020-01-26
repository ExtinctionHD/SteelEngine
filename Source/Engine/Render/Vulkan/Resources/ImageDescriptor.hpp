#pragma once

enum class eImageDescriptorType
{
    kUninitialized,
    kImageOnly,
    kImageWithView,
};

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

class ImageDescriptor
{
public:
    eImageDescriptorType GetType() const { return type; }
    const ImageDescription &GetProperties() const { return description; }

    vk::Image GetImage() const { return image; }
    vk::ImageView GetView() const { return view; }
    vk::DeviceMemory GetMemory() const { return memory; }

    bool operator ==(const ImageDescriptor &other) const;

private:
    ImageDescriptor() = default;

    eImageDescriptorType type = eImageDescriptorType::kUninitialized;
    ImageDescription description = {};

    vk::Image image;
    vk::ImageView view;
    vk::DeviceMemory memory;

    friend class ImageManager;
};
