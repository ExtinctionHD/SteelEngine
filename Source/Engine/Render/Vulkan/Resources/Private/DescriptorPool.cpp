#include "Engine/Render/Vulkan/Resources/DescriptorPool.hpp"

#include "Utils/Assert.hpp"

namespace SDescriptorPool
{
    std::vector<vk::DescriptorSetLayoutBinding> ObtainBindings(const std::vector<DescriptorProperties> &properties)
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings(properties.size());

        for (uint32_t i = 0; i < properties.size(); ++i)
        {
            bindings[i] = vk::DescriptorSetLayoutBinding(i,
                    properties[i].type, properties[i].count, properties[i].stages);
        }

        return bindings;
    }
}

std::unique_ptr<DescriptorPool> DescriptorPool::Create(std::shared_ptr<Device> device)
{
    const std::vector<vk::DescriptorPoolSize> poolSizes{
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000)
    };

    const vk::DescriptorPoolCreateInfo createInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000,
            static_cast<uint32_t>(poolSizes.size()), poolSizes.data());

    const auto [result, descriptorPool] = device->Get().createDescriptorPool(createInfo);
    Assert(result == vk::Result::eSuccess);

    return std::make_unique<DescriptorPool>(device, descriptorPool);
}

DescriptorPool::DescriptorPool(std::shared_ptr<Device> aDevice, vk::DescriptorPool aDescriptorPool)
    : device(aDevice)
    , descriptorPool(aDescriptorPool)
{}

DescriptorPool::~DescriptorPool()
{
    for (auto &[layout, sets] : descriptorSets)
    {
        device->Get().destroyDescriptorSetLayout(layout);
    }
    device->Get().destroyDescriptorPool(descriptorPool);
}

vk::DescriptorSetLayout DescriptorPool::CreateDescriptorSetLayout(
        const std::vector<DescriptorProperties> &properties)
{
    const std::vector<vk::DescriptorSetLayoutBinding> bindings = SDescriptorPool::ObtainBindings(properties);
    const vk::DescriptorSetLayoutCreateInfo createInfo({}, static_cast<uint32_t>(bindings.size()), bindings.data());

    const auto [result, layout] = device->Get().createDescriptorSetLayout(createInfo);
    Assert(result == vk::Result::eSuccess);

    descriptorSets[layout];

    return layout;
}

std::vector<vk::DescriptorSet> DescriptorPool::AllocateDescriptorSets(
        const std::vector<vk::DescriptorSetLayout> &layouts)
{
    const vk::DescriptorSetAllocateInfo allocateInfo(descriptorPool,
            static_cast<uint32_t>(layouts.size()), layouts.data());

    const auto [result, allocatedSets] = device->Get().allocateDescriptorSets(allocateInfo);
    Assert(result == vk::Result::eSuccess);

    for (uint32_t i = 0; i < layouts.size(); ++i)
    {
        descriptorSets[layouts[i]].push_back(allocatedSets[i]);
    }

    return allocatedSets;
}

vk::DescriptorSet DescriptorPool::AllocateDescriptorSet(vk::DescriptorSetLayout layout)
{
    return AllocateDescriptorSets({ layout }).front();
}
