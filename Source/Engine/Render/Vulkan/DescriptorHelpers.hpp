#pragma once

#include "Utils/Helpers.hpp"

struct DescriptorDescription
{
    vk::DescriptorType type;
    uint32_t count;

    vk::ShaderStageFlags stageFlags;
    vk::DescriptorBindingFlags bindingFlags;

    bool operator==(const DescriptorDescription &other) const;
};

using DescriptorSetDescription = std::vector<DescriptorDescription>;

using ImagesInfo = std::vector<vk::DescriptorImageInfo>;
using BuffersInfo = std::vector<vk::DescriptorBufferInfo>;
using BufferViews = std::vector<vk::BufferView>;
using AccelerationStructuresInfo = vk::WriteDescriptorSetAccelerationStructureNV;

using DescriptorsInfo = std::variant<ImagesInfo, BuffersInfo, BufferViews, AccelerationStructuresInfo>;

struct DescriptorData
{
    vk::DescriptorType type;
    DescriptorsInfo descriptorsInfo;
};

using DescriptorSetData = std::vector<DescriptorData>;

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
