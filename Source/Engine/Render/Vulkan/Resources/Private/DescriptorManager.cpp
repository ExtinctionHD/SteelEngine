#include "Engine/Render/Vulkan/Resources/DescriptorManager.hpp"

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

std::unique_ptr<DescriptorManager> DescriptorManager::Create(uint32_t maxSetCount,
        const std::vector<vk::DescriptorPoolSize>& poolSizes)
{
    constexpr vk::DescriptorPoolCreateFlags createFlags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind; // TODO implement separate pool

    const vk::DescriptorPoolCreateInfo createInfo(createFlags, maxSetCount,
            static_cast<uint32_t>(poolSizes.size()), poolSizes.data());

    const auto [result, descriptorPool] = VulkanContext::device->Get().createDescriptorPool(createInfo);
    Assert(result == vk::Result::eSuccess);

    return std::unique_ptr<DescriptorManager>(new DescriptorManager(descriptorPool));
}

DescriptorManager::DescriptorManager(vk::DescriptorPool descriptorPool_)
    : descriptorPool(descriptorPool_)
{}

DescriptorManager::~DescriptorManager()
{
    for (const auto& [description, layout] : layoutCache)
    {
        VulkanContext::device->Get().destroyDescriptorSetLayout(layout);
    }

    VulkanContext::device->Get().destroyDescriptorPool(descriptorPool);
}

vk::DescriptorSetLayout DescriptorManager::GetDescriptorSetLayout(const DescriptorSetDescription& description)
{
    const auto it = layoutCache.find(description);

    if (it != layoutCache.end())
    {
        return it->second;
    }

    constexpr vk::DescriptorSetLayoutCreateFlags createFlags =
            vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;

    const std::vector<vk::DescriptorSetLayoutBinding> bindings = Details::GetBindings(description);
    const std::vector<vk::DescriptorBindingFlags> bindingFlags = Details::GetBindingFlags(description);

    const vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo(
            static_cast<uint32_t>(bindingFlags.size()), bindingFlags.data());

    vk::StructureChain<vk::DescriptorSetLayoutCreateInfo, vk::DescriptorSetLayoutBindingFlagsCreateInfo> structures(
            vk::DescriptorSetLayoutCreateInfo(createFlags, static_cast<uint32_t>(bindings.size()), bindings.data()),
            bindingFlagsCreateInfo);

    const vk::DescriptorSetLayoutCreateInfo& createInfo = structures.get<vk::DescriptorSetLayoutCreateInfo>();

    const auto [result, layout] = VulkanContext::device->Get().createDescriptorSetLayout(createInfo);
    Assert(result == vk::Result::eSuccess);

    layoutCache[description] = layout;

    return layout;
}

std::vector<vk::DescriptorSet> DescriptorManager::AllocateDescriptorSets(
        const std::vector<vk::DescriptorSetLayout>& layouts) const
{
    const vk::DescriptorSetAllocateInfo allocateInfo(descriptorPool,
            static_cast<uint32_t>(layouts.size()), layouts.data());

    const auto [result, allocatedSets] = VulkanContext::device->Get().allocateDescriptorSets(allocateInfo);
    Assert(result == vk::Result::eSuccess);

    return allocatedSets;
}

std::vector<vk::DescriptorSet> DescriptorManager::AllocateDescriptorSets(
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

void DescriptorManager::FreeDescriptorSets(const std::vector<vk::DescriptorSet>& sets) const
{
    VulkanContext::device->Get().freeDescriptorSets(descriptorPool, sets);
}

void DescriptorManager::UpdateDescriptorSet(const std::vector<vk::WriteDescriptorSet>& writes)
{
    VulkanContext::device->Get().updateDescriptorSets(writes, {});
}
