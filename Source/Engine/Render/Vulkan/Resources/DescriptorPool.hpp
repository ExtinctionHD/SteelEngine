#pragma once

#include <map>

#include "Engine/Render/Vulkan/Device.hpp"

#include "Engine/Render/Vulkan/Resources/DescriptorData.hpp"

class DescriptorPool
{
public:
    static std::unique_ptr<DescriptorPool> Create(std::shared_ptr<Device> device);

    DescriptorPool(std::shared_ptr<Device> aDevice, vk::DescriptorPool aDescriptorPool);
    ~DescriptorPool();

    vk::DescriptorSetLayout CreateDescriptorSetLayout(DescriptorSetProperties &properties);

    std::vector<vk::DescriptorSet> AllocateDescriptorSets(
            const std::vector<vk::DescriptorSetLayout> &layouts) const;

    vk::DescriptorSet AllocateDescriptorSet(vk::DescriptorSetLayout layout) const;

    void UpdateDescriptorSet(vk::DescriptorSet descriptorSet,
            const DescriptorSetData &descriptorSetData, uint32_t bindingOffset = 0) const;

private:
    struct LayoutsCacheEntry
    {
        DescriptorSetProperties properties;
        vk::DescriptorSetLayout layout;

        bool operator ==(const LayoutsCacheEntry &other) const;
    };

    std::shared_ptr<Device> device;

    vk::DescriptorPool descriptorPool;

    std::vector<LayoutsCacheEntry> layoutsCache;
};
