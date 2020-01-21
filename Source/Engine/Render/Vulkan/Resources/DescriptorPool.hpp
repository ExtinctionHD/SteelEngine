#pragma once

#include <set>

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorTypes.hpp"

class DescriptorPool
{
public:
    static std::unique_ptr<DescriptorPool> Create(std::shared_ptr<Device> device,
            const std::set<vk::DescriptorType> &descriptorTypes);

    DescriptorPool(std::shared_ptr<Device> aDevice, vk::DescriptorPool aDescriptorPool);
    ~DescriptorPool();

    vk::DescriptorSetLayout CreateDescriptorSetLayout(const DescriptorSetProperties &properties);

    void DestroyDescriptorSetLayout(vk::DescriptorSetLayout layout);

    std::vector<vk::DescriptorSet> AllocateDescriptorSets(
            const std::vector<vk::DescriptorSetLayout> &layouts) const;

    vk::DescriptorSet AllocateDescriptorSet(vk::DescriptorSetLayout layout) const;

    void UpdateDescriptorSet(vk::DescriptorSet descriptorSet,
            const DescriptorSetData &descriptorSetData, uint32_t bindingOffset = 0);

    void PerformUpdate();

private:
    struct LayoutsCacheEntry
    {
        DescriptorSetProperties properties;
        vk::DescriptorSetLayout layout;
    };

    std::shared_ptr<Device> device;

    vk::DescriptorPool descriptorPool;

    std::list<LayoutsCacheEntry> layoutsCache;

    std::vector<vk::WriteDescriptorSet> descriptorWrites;
};
