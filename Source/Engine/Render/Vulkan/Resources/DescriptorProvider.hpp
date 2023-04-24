#pragma once

#include "Engine/Render/Vulkan/Resources/DescriptorHelpers.hpp"

enum class DescriptorSetRate
{
    eGlobal,
    ePerFrame
};

class DescriptorProvider
{
public:
    ~DescriptorProvider();
    
    void Allocate(const std::vector<vk::DescriptorSetLayout>& layouts_, 
            const std::vector<DescriptorSetRate>& rates, uint32_t copyCount);

    void AllocateStandard(const std::vector<vk::DescriptorSetLayout>& layouts_);

    void AllocateSingleCopy(const std::vector<vk::DescriptorSetLayout>& layouts_);

    void UpdateDescriptorSet(uint32_t copyIndex, uint32_t setIndex, const DescriptorSetData& data) const;
    
    const std::vector<vk::DescriptorSet>& GetDescriptorSets(uint32_t copyIndex) const;

    uint32_t GetCopyCount() const;

private:
    static const std::vector<DescriptorSetRate> kStandardRates;

    std::vector<vk::DescriptorSetLayout> layouts;

    std::vector<std::vector<vk::DescriptorSet>> descriptorCopies;

    std::vector<vk::DescriptorSet> descriptors;
};
