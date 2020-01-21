#pragma once

#include <variant>

struct DescriptorProperties
{
    vk::DescriptorType type;
    vk::ShaderStageFlags stageFlags;

    bool operator ==(const DescriptorProperties &other) const;
};

using DescriptorSetProperties = std::vector<DescriptorProperties>;

using DescriptorInfo = std::variant<vk::DescriptorBufferInfo, vk::DescriptorImageInfo, vk::BufferView>;

struct DescriptorData
{
    vk::DescriptorType type;
    DescriptorInfo info;
};

using DescriptorSetData = std::vector<DescriptorData>;
