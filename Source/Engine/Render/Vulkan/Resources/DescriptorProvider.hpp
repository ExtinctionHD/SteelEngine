#pragma once

#include "Engine/Render/Vulkan/Resources/DescriptorHelpers.hpp"

enum class DescriptorSetRate
{
    eGlobal,
    ePerSlice
};

using DescriptorSlice = std::vector<vk::DescriptorSet>;

class DescriptorProvider
{
public:
    DescriptorProvider(const std::vector<vk::DescriptorSetLayout>& layouts_,
            const std::vector<DescriptorSetRate>& rates, uint32_t sliceCount);

    ~DescriptorProvider();

    void UpdateDescriptorSet(uint32_t sliceIndex, uint32_t setIndex, const DescriptorSetData& data) const;

    const DescriptorSlice& GetDescriptorSlice(uint32_t sliceIndex) const;

    uint32_t GetSliceCount() const;

    uint32_t GetSetCount() const;

private:
    std::vector<vk::DescriptorSetLayout> layouts;

    std::vector<DescriptorSlice> descriptorSlices;

    std::vector<vk::DescriptorSet> descriptors;
};

class FlatDescriptorProvider : DescriptorProvider
{
public:
    FlatDescriptorProvider(const std::vector<vk::DescriptorSetLayout>& layouts_);

    void UpdateDescriptorSet(uint32_t setIndex, const DescriptorSetData& data) const;

    using DescriptorProvider::GetDescriptorSlice;

    using DescriptorProvider::GetSliceCount;

    using DescriptorProvider::GetSetCount;
};

class FrameDescriptorProvider : DescriptorProvider
{
public:
    FrameDescriptorProvider(const std::vector<vk::DescriptorSetLayout>& layouts_);

    void UpdateGlobalDescriptorSet(const DescriptorSetData& data) const;

    void UpdateFrameDescriptorSet(uint32_t sliceIndex, const DescriptorSetData& data) const;

    using DescriptorProvider::GetDescriptorSlice;

    using DescriptorProvider::GetSliceCount;

    using DescriptorProvider::GetSetCount;

private:
    static const std::vector<DescriptorSetRate> kRates;

    static constexpr uint32_t kGlobalSetIndex = 0;
    static constexpr uint32_t kFrameSetIndex = 1;
};

class FrameOnlyDescriptorProvider : DescriptorProvider
{
public:
    FrameOnlyDescriptorProvider(const std::vector<vk::DescriptorSetLayout>& layouts_);

    void UpdateFrameDescriptorSet(uint32_t sliceIndex, const DescriptorSetData& data) const;

    using DescriptorProvider::GetDescriptorSlice;

    using DescriptorProvider::GetSliceCount;

    using DescriptorProvider::GetSetCount;

private:
    static constexpr uint32_t kFrameSetIndex = 0;
};
