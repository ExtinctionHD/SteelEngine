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

    vk::DescriptorSetLayout CreateDescriptorSetLayout(
            const std::vector<DescriptorProperties> &properties);

    std::vector<vk::DescriptorSet> AllocateDescriptorSets(
            const std::vector<vk::DescriptorSetLayout> &layouts);

    vk::DescriptorSet AllocateDescriptorSet(vk::DescriptorSetLayout layout);

private:
    std::shared_ptr<Device> device;

    vk::DescriptorPool descriptorPool;

    std::map<vk::DescriptorSetLayout, std::list<vk::DescriptorSet>> descriptorSets;
};
