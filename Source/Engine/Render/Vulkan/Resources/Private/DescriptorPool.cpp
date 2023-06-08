#include <experimental/unordered_map>

#include "Engine/Render/Vulkan/Resources/DescriptorPool.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"

namespace Details
{
    static std::vector<vk::DescriptorSetLayoutBinding> GetBindings(
            const DescriptorSetDescription& descriptorSetDescription)
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;

        for (const auto& descriptorDescription : descriptorSetDescription)
        {
            const vk::DescriptorSetLayoutBinding binding(
                    descriptorDescription.key.binding,
                    descriptorDescription.type,
                    descriptorDescription.count,
                    descriptorDescription.stageFlags);

            bindings.push_back(binding);
        }

        return bindings;
    }

    static std::vector<vk::DescriptorBindingFlags> GetBindingFlags(
            const DescriptorSetDescription& descriptorSetDescription)
    {
        std::vector<vk::DescriptorBindingFlags> bindingFlags;

        for (const auto& descriptorDescription : descriptorSetDescription)
        {
            bindingFlags.push_back(descriptorDescription.bindingFlags);
        }

        return bindingFlags;
    }
}

std::unique_ptr<DescriptorPool> DescriptorPool::Create(uint32_t maxSetCount,
        const std::vector<vk::DescriptorPoolSize>& poolSizes)
{
    const vk::DescriptorPoolCreateInfo createInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, maxSetCount,
            static_cast<uint32_t>(poolSizes.size()), poolSizes.data());

    const auto [result, descriptorPool] = VulkanContext::device->Get().createDescriptorPool(createInfo);
    Assert(result == vk::Result::eSuccess);

    return std::unique_ptr<DescriptorPool>(new DescriptorPool(descriptorPool));
}

DescriptorPool::DescriptorPool(vk::DescriptorPool descriptorPool_)
    : descriptorPool(descriptorPool_)
{}

DescriptorPool::~DescriptorPool()
{
    VulkanContext::device->Get().destroyDescriptorPool(descriptorPool);
}

vk::DescriptorSetLayout DescriptorPool::CreateDescriptorSetLayout(const DescriptorSetDescription& description) const
{
    const std::vector<vk::DescriptorSetLayoutBinding> bindings = Details::GetBindings(description);
    const std::vector<vk::DescriptorBindingFlags> bindingFlags = Details::GetBindingFlags(description);

    const vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo(
            static_cast<uint32_t>(bindingFlags.size()), bindingFlags.data());

    vk::StructureChain<vk::DescriptorSetLayoutCreateInfo, vk::DescriptorSetLayoutBindingFlagsCreateInfo> structures(
            vk::DescriptorSetLayoutCreateInfo({}, static_cast<uint32_t>(bindings.size()), bindings.data()),
            bindingFlagsCreateInfo);

    const vk::DescriptorSetLayoutCreateInfo& createInfo = structures.get<vk::DescriptorSetLayoutCreateInfo>();

    const auto [result, layout] = VulkanContext::device->Get().createDescriptorSetLayout(createInfo);
    Assert(result == vk::Result::eSuccess);

    return layout;
}

void DescriptorPool::DestroyDescriptorSetLayout(vk::DescriptorSetLayout layout)
{
    VulkanContext::device->Get().destroyDescriptorSetLayout(layout);
}

std::vector<vk::DescriptorSet> DescriptorPool::AllocateDescriptorSets(
        const std::vector<vk::DescriptorSetLayout>& layouts) const
{
    const vk::DescriptorSetAllocateInfo allocateInfo(descriptorPool,
            static_cast<uint32_t>(layouts.size()), layouts.data());

    const auto [result, allocatedSets] = VulkanContext::device->Get().allocateDescriptorSets(allocateInfo);
    Assert(result == vk::Result::eSuccess);

    return allocatedSets;
}

std::vector<vk::DescriptorSet> DescriptorPool::AllocateDescriptorSets(
        const std::vector<vk::DescriptorSetLayout>& layouts,
        const std::vector<uint32_t>& descriptorCounts) const
{
    const vk::DescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountAllocateInfo(
            static_cast<uint32_t>(descriptorCounts.size()), descriptorCounts.data());

    vk::StructureChain<vk::DescriptorSetAllocateInfo, vk::DescriptorSetVariableDescriptorCountAllocateInfo> structures(
            vk::DescriptorSetAllocateInfo(descriptorPool, static_cast<uint32_t>(layouts.size()), layouts.data()),
            variableDescriptorCountAllocateInfo);

    const vk::DescriptorSetAllocateInfo& allocateInfo = structures.get<vk::DescriptorSetAllocateInfo>();

    const auto [result, allocatedSets] = VulkanContext::device->Get().allocateDescriptorSets(allocateInfo);
    Assert(result == vk::Result::eSuccess);

    return allocatedSets;
}

void DescriptorPool::FreeDescriptorSets(const std::vector<vk::DescriptorSet>& sets) const
{
    VulkanContext::device->Get().freeDescriptorSets(descriptorPool, sets);
}

void DescriptorPool::UpdateDescriptorSet(const std::vector<vk::WriteDescriptorSet>& writes)
{
    VulkanContext::device->Get().updateDescriptorSets(writes, {});
}
