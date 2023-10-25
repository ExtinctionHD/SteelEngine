#pragma once

#include "Engine/Render/RenderContext.hpp"

struct TextureSampler;

struct DescriptorKey
{
    uint32_t set;
    uint32_t binding;

    bool operator==(const DescriptorKey& other) const;
    bool operator<(const DescriptorKey& other) const;
};

struct DescriptorDescription
{
    DescriptorKey key;
    uint32_t count;
    vk::DescriptorType type;
    vk::ShaderStageFlags stageFlags;
    vk::DescriptorBindingFlags bindingFlags;

    bool operator==(const DescriptorDescription& other) const;

    bool operator<(const DescriptorDescription& other) const;
};

using DescriptorSetDescription = std::vector<DescriptorDescription>;

using ImageInfo = std::vector<vk::DescriptorImageInfo>;
using BufferInfo = std::vector<vk::DescriptorBufferInfo>;
using BufferViews = std::vector<vk::BufferView>;
using AccelerationStructureInfo = vk::WriteDescriptorSetAccelerationStructureKHR;

using DescriptorInfo
    = std::variant<std::monostate, ImageInfo, BufferInfo, BufferViews, AccelerationStructureInfo>;

using DescriptorSource = std::variant<std::monostate, vk::Sampler, vk::ImageView, TextureSampler, vk::Buffer, vk::BufferView, const vk::AccelerationStructureKHR*>;

using DescriptorSources = std::variant<std::monostate, const std::vector<vk::Sampler>*, const std::vector<vk::ImageView>*, const std::vector<TextureSampler>*, const std::vector<vk::Buffer>*, const std::vector<vk::BufferView>*, const std::vector<vk::AccelerationStructureKHR>*>;

struct DescriptorData
{
    vk::DescriptorType type;
    DescriptorInfo descriptorInfo;
};

namespace DescriptorHelpers
{
    DescriptorData GetData(vk::DescriptorType type, const DescriptorSource& source);

    DescriptorData GetData(vk::Sampler sampler);

    DescriptorData GetData(vk::ImageView view, vk::Sampler sampler = RenderContext::defaultSampler);

    DescriptorData GetData(const TextureSampler& textureSampler);

    DescriptorData GetData(vk::Buffer buffer);

    DescriptorData GetData(const vk::AccelerationStructureKHR& accelerationStructure);

    DescriptorData GetStorageData(vk::ImageView view);

    DescriptorData GetStorageData(vk::Buffer buffer);

    DescriptorData GetData(vk::DescriptorType type, const DescriptorSources& sources);

    DescriptorData GetData(
        const std::vector<vk::ImageView>& views, vk::Sampler sampler = RenderContext::defaultSampler);

    DescriptorData GetData(const std::vector<TextureSampler>& textureSamplers);

    DescriptorData GetData(const std::vector<vk::Buffer>& buffers);

    DescriptorData GetStorageData(const std::vector<vk::ImageView>& views);

    DescriptorData GetStorageData(const std::vector<vk::Buffer>& buffers);

    bool WriteDescriptorData(vk::WriteDescriptorSet& write, const DescriptorData& data);
}
