#pragma once

struct DescriptorDescription
{
    vk::DescriptorType type;
    uint32_t count;

    vk::ShaderStageFlags stageFlags;
    vk::DescriptorBindingFlags bindingFlags;

    bool operator==(const DescriptorDescription &other) const;
};

using DescriptorSetDescription = std::vector<DescriptorDescription>;

using ImageInfo = std::vector<vk::DescriptorImageInfo>;
using BufferInfo = std::vector<vk::DescriptorBufferInfo>;
using BufferViews = std::vector<vk::BufferView>;
using AccelerationStructureInfo = vk::WriteDescriptorSetAccelerationStructureNV;

using DescriptorInfo = std::variant<ImageInfo, BufferInfo, BufferViews, AccelerationStructureInfo>;

struct DescriptorData
{
    vk::DescriptorType type;
    DescriptorInfo descriptorInfo;
};

using DescriptorSetData = std::vector<DescriptorData>;

struct SingleDescriptor
{
    vk::DescriptorSetLayout layout;
    vk::DescriptorSet descriptorSet;
};

struct IndexedDescriptor
{
    vk::DescriptorSetLayout layout;
    vk::DescriptorSet descriptorSet;
};

namespace DescriptorHelpers
{
    ImageInfo GetInfo(vk::Sampler sampler, vk::ImageView view);

    ImageInfo GetInfo(vk::ImageView view);

    BufferInfo GetInfo(vk::Buffer buffer);

    AccelerationStructureInfo GetInfo(const vk::AccelerationStructureNV &accelerationStructure);
}
