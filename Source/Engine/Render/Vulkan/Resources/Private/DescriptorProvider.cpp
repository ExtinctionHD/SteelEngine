#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace Details
{
    vk::DescriptorSet AllocateDescriptorSet(vk::DescriptorSetLayout layout)
    {
        return VulkanContext::descriptorPool->AllocateDescriptorSets({ layout }).front();
    }
}

const std::vector<DescriptorSetRate> DescriptorProvider::kStandardRates{
    DescriptorSetRate::eGlobal, DescriptorSetRate::ePerFrame
};

void DescriptorProvider::Allocate(const std::vector<vk::DescriptorSetLayout>& layouts_, 
            const std::vector<DescriptorSetRate>& rates, uint32_t copyCount)
{
    layouts = layouts_;

    Assert(layouts.size() == rates.size());

    descriptorCopies.resize(copyCount);

    for (auto& descriptorSets : descriptorCopies)
    {
        descriptorSets.resize(layouts.size());
    }

    descriptors.reserve(layouts.size());
    
    for (size_t i = 0; i < rates.size(); ++i)
    {
        const DescriptorSetRate rate = rates[i];

        if (rate == DescriptorSetRate::eGlobal)
        {
            const vk::DescriptorSet descriptorSet = Details::AllocateDescriptorSet(layouts[i]);

            for (auto& descriptorSets : descriptorCopies)
            {
                descriptorSets[i] = descriptorSet;
            }

            descriptors.push_back(descriptorSet);
        }
        else
        {
            Assert(rate == DescriptorSetRate::ePerFrame);
            
            for (auto& descriptorSets : descriptorCopies)
            {
                descriptorSets[i] = Details::AllocateDescriptorSet(layouts[i]);

                descriptors.push_back(descriptorSets[i]);
            }
        }
    }
}

void DescriptorProvider::AllocateStandard(const std::vector<vk::DescriptorSetLayout>& layouts_)
{
    Allocate(layouts_, kStandardRates, VulkanContext::swapchain->GetImageCount());
}

void DescriptorProvider::AllocateSingleCopy(const std::vector<vk::DescriptorSetLayout>& layouts_)
{
    layouts = layouts_;

    descriptorCopies.push_back(VulkanContext::descriptorPool->AllocateDescriptorSets(layouts));

    descriptors = descriptorCopies.front();
}

DescriptorProvider::~DescriptorProvider()
{
    if (!descriptors.empty())
    {
        VulkanContext::descriptorPool->FreeDescriptorSets(descriptors);
    }
}

void DescriptorProvider::UpdateDescriptorSet(uint32_t copyIndex, uint32_t setIndex, const DescriptorSetData& data) const
{
    VulkanContext::descriptorPool->UpdateDescriptorSet(descriptorCopies[copyIndex][setIndex], data, 0);
}

const std::vector<vk::DescriptorSet>& DescriptorProvider::GetDescriptorSets(uint32_t copyIndex) const
{
    return descriptorCopies[copyIndex];
}

uint32_t DescriptorProvider::GetCopyCount() const
{
    return static_cast<uint32_t>(descriptorCopies.size());
}
