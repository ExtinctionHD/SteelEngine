#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace Details
{
    vk::DescriptorSet AllocateDescriptorSet(vk::DescriptorSetLayout layout)
    {
        return VulkanContext::descriptorPool->AllocateDescriptorSets({ layout }).front();
    }
}

void DescriptorProvider::Allocate(const std::vector<vk::DescriptorSetLayout>& layouts_,
        const std::vector<DescriptorSetRate>& rates, uint32_t sliceCount)
{
    if (!descriptors.empty())
    {
        VulkanContext::descriptorPool->FreeDescriptorSets(descriptors);
    }

    layouts = layouts_;

    Assert(layouts.size() == rates.size());

    descriptorSlices.resize(sliceCount);

    for (auto& descriptorSlice : descriptorSlices)
    {
        descriptorSlice.resize(layouts.size());
    }

    descriptors.reserve(layouts.size());

    for (size_t i = 0; i < rates.size(); ++i)
    {
        const DescriptorSetRate rate = rates[i];

        if (rate == DescriptorSetRate::eGlobal)
        {
            const vk::DescriptorSet descriptorSet = Details::AllocateDescriptorSet(layouts[i]);

            for (auto& descriptorSlice : descriptorSlices)
            {
                descriptorSlice[i] = descriptorSet;
            }

            descriptors.push_back(descriptorSet);
        }
        else
        {
            Assert(rate == DescriptorSetRate::ePerSlice);

            for (auto& descriptorSlice : descriptorSlices)
            {
                descriptorSlice[i] = Details::AllocateDescriptorSet(layouts[i]);

                descriptors.push_back(descriptorSlice[i]);
            }
        }
    }
}

DescriptorProvider::~DescriptorProvider()
{
    if (!descriptors.empty())
    {
        VulkanContext::descriptorPool->FreeDescriptorSets(descriptors);
    }
}

void DescriptorProvider::UpdateDescriptorSet(uint32_t sliceIndex, uint32_t setIndex,
        const DescriptorSetData& data) const
{
    Assert(sliceIndex < GetSliceCount());
    Assert(setIndex < GetSetCount());

    VulkanContext::descriptorPool->UpdateDescriptorSet(descriptorSlices[sliceIndex][setIndex], data, 0);
}

const std::vector<vk::DescriptorSet>& DescriptorProvider::GetDescriptorSlice(uint32_t sliceIndex) const
{
    Assert(sliceIndex < GetSliceCount());

    return descriptorSlices[sliceIndex];
}

uint32_t DescriptorProvider::GetSliceCount() const
{
    return static_cast<uint32_t>(descriptorSlices.size());
}

uint32_t DescriptorProvider::GetSetCount() const
{
    Assert(!descriptorSlices.empty());

    return static_cast<uint32_t>(descriptorSlices.front().size());
}

void FlatDescriptorProvider::Allocate(const std::vector<vk::DescriptorSetLayout>& layouts_)
{
    Allocate(layouts_, Repeat(DescriptorSetRate::eGlobal, layouts_.size()), 1);
}

void FlatDescriptorProvider::UpdateDescriptorSet(uint32_t setIndex, const DescriptorSetData& data) const
{
    UpdateDescriptorSet(0, setIndex, data);
}

void FrameDescriptorProvider::Allocate(const std::vector<vk::DescriptorSetLayout>& layouts_)
{
    Allocate(layouts_, kRates, VulkanContext::swapchain->GetImageCount());
}

void FrameDescriptorProvider::UpdateGlobalDescriptorSet(const DescriptorSetData& data) const
{
    UpdateDescriptorSet(0, kGlobalSetIndex, data);
}

void FrameDescriptorProvider::UpdateFrameDescriptorSet(uint32_t sliceIndex, const DescriptorSetData& data) const
{
    UpdateDescriptorSet(sliceIndex, kFrameSetIndex, data);
}

const std::vector<DescriptorSetRate> FrameDescriptorProvider::kRates{
    DescriptorSetRate::eGlobal, DescriptorSetRate::ePerSlice
};

void FrameOnlyDescriptorProvider::Allocate(const std::vector<vk::DescriptorSetLayout>& layouts_)
{
    Allocate(layouts_, { DescriptorSetRate::eGlobal }, VulkanContext::swapchain->GetImageCount());
}

void FrameOnlyDescriptorProvider::UpdateFrameDescriptorSet(uint32_t sliceIndex, const DescriptorSetData& data) const
{
    UpdateDescriptorSet(sliceIndex, kFrameSetIndex, data);
}
