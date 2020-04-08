#pragma once

#include "Utils/Helpers.hpp"

struct Texture;

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

namespace DescriptorHelpers
{
    ImageInfo GetInfo(const Texture &texture);

    BufferInfo GetInfo(vk::Buffer buffer);

    AccelerationStructureInfo GetInfo(const vk::AccelerationStructureNV &accelerationStructure);
}

namespace std
{
    template <>
    struct hash<DescriptorDescription>
    {
        size_t operator()(const DescriptorDescription &description) const noexcept
        {
            size_t result = 0;

            CombineHash(result, description.type);
            CombineHash(result, description.count);
            CombineHash(result, static_cast<uint32_t>(description.stageFlags));
            CombineHash(result, static_cast<uint32_t>(description.bindingFlags));

            return result;
        }
    };

    template <>
    struct hash<DescriptorSetDescription>
    {
        size_t operator()(const DescriptorSetDescription &descriptorSetDescription) const noexcept
        {
            size_t result = 0;

            for (const auto &descriptorDescription : descriptorSetDescription)
            {
                CombineHash(result, descriptorDescription);
            }

            return result;
        }
    };
}
