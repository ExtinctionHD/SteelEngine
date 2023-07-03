#pragma once

#include "Engine/Render/Vulkan/Resources/DescriptorHelpers.hpp"

class DescriptorManager
{
public:
    static std::unique_ptr<DescriptorManager> Create(uint32_t maxSetCount,
            const std::vector<vk::DescriptorPoolSize>& poolSizes);

    ~DescriptorManager();

    vk::DescriptorSetLayout CreateDescriptorSetLayout(const DescriptorSetDescription& description);

    std::vector<vk::DescriptorSet> AllocateDescriptorSets(const std::vector<vk::DescriptorSetLayout>& layouts) const;

    std::vector<vk::DescriptorSet> AllocateDescriptorSets(const std::vector<vk::DescriptorSetLayout>& layouts,
            const std::vector<uint32_t>& descriptorCounts) const;

    void FreeDescriptorSets(const std::vector<vk::DescriptorSet>& sets) const;

    void UpdateDescriptorSet(const std::vector<vk::WriteDescriptorSet>& writes);

private:
    vk::DescriptorPool descriptorPool;

    std::map<DescriptorSetDescription, vk::DescriptorSetLayout> layoutCache;

    DescriptorManager(vk::DescriptorPool descriptorPool_);
};
