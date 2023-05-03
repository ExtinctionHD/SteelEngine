#pragma once

#include "Engine/Render/Vulkan/Resources/DescriptorHelpers.hpp"

// TODO rename to DescriptorManager, implement DescriptorSetLayouts cache
class DescriptorPool
{
public:
    static std::unique_ptr<DescriptorPool> Create(uint32_t maxSetCount,
            const std::vector<vk::DescriptorPoolSize>& poolSizes);

    ~DescriptorPool();

    vk::DescriptorSetLayout CreateDescriptorSetLayout(const DescriptorSetDescription& description) const;

    void DestroyDescriptorSetLayout(vk::DescriptorSetLayout layout);

    std::vector<vk::DescriptorSet> AllocateDescriptorSets(const std::vector<vk::DescriptorSetLayout>& layouts) const;

    std::vector<vk::DescriptorSet> AllocateDescriptorSets(const std::vector<vk::DescriptorSetLayout>& layouts,
            const std::vector<uint32_t>& descriptorCounts) const;

    void FreeDescriptorSets(const std::vector<vk::DescriptorSet>& descriptorSets) const;

    void UpdateDescriptorSet(vk::DescriptorSet descriptorSet,
            const DescriptorSetData& descriptorSetData, uint32_t bindingOffset) const;

private:
    vk::DescriptorPool descriptorPool;

    DescriptorPool(vk::DescriptorPool descriptorPool_);
};
