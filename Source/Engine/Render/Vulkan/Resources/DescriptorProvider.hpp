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
    virtual ~DescriptorProvider();

    void Allocate(const std::vector<vk::DescriptorSetLayout>& layouts_,
            const std::vector<DescriptorSetRate>& rates, uint32_t sliceCount);

    void FreeDescriptors();

    void UpdateDescriptorSet(uint32_t sliceIndex, uint32_t setIndex, const DescriptorSetData& data) const;

    const DescriptorSlice& GetDescriptorSlice(uint32_t sliceIndex) const;

    uint32_t GetSliceCount() const;

    uint32_t GetSetCount() const;

private:
    std::vector<vk::DescriptorSetLayout> layouts;

    std::vector<DescriptorSlice> descriptorSlices;

    std::vector<vk::DescriptorSet> descriptors;
};

class FlatDescriptorProvider : public DescriptorProvider
{
public:
    void Allocate(const std::vector<vk::DescriptorSetLayout>& layouts_);

    void UpdateDescriptorSet(uint32_t setIndex, const DescriptorSetData& data) const;

private:
    using DescriptorProvider::Allocate;

    using DescriptorProvider::UpdateDescriptorSet;
};

class FrameDescriptorProvider : public DescriptorProvider
{
public:
    void Allocate(const std::vector<vk::DescriptorSetLayout>& layouts_);

    void UpdateGlobalDescriptorSet(const DescriptorSetData& data) const;

    void UpdateFrameDescriptorSet(uint32_t sliceIndex, const DescriptorSetData& data) const;

private:
    static const std::vector<DescriptorSetRate> kRates;

    static constexpr uint32_t kGlobalSetIndex = 0;
    static constexpr uint32_t kFrameSetIndex = 1;

    using DescriptorProvider::Allocate;

    using DescriptorProvider::UpdateDescriptorSet;
};

class FrameOnlyDescriptorProvider : public DescriptorProvider
{
public:
    void Allocate(const std::vector<vk::DescriptorSetLayout>& layouts_);

    void UpdateFrameDescriptorSet(uint32_t sliceIndex, const DescriptorSetData& data) const;

private:
    static constexpr uint32_t kFrameSetIndex = 0;

    using DescriptorProvider::Allocate;

    using DescriptorProvider::UpdateDescriptorSet;
};
