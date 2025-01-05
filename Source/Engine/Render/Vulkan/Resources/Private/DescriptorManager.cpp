#include "Engine/Render/Vulkan/Resources/DescriptorManager.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"

namespace Details
{
    static int maxDescriptorSetCount = 512;
    static CVarInt maxDescriptorSetCountCVar(
        "vk.MaxDescriptorSetCount", maxDescriptorSetCount);

    static int maxUniformBufferCount = 2048;
    static CVarInt maxUniformBufferCountCVar(
        "vk.MaxDescriptorCount.UniformBuffer", maxUniformBufferCount);

    static int maxStorageBufferCount = 2048;
    static CVarInt maxStorageBufferCountCVar(
        "vk.MaxDescriptorCount.StorageBuffer", maxStorageBufferCount);

    static int maxStorageImageCount = 2048;
    static CVarInt maxStorageImageCountCVar(
        "vk.MaxDescriptorCount.StorageImage", maxStorageImageCount);

    static int maxCombinedImageSamplerCount = 2048;
    static CVarInt maxCombinedImageSamplerCountCVar(
        "vk.MaxDescriptorCount.CombinedImageSampler", maxCombinedImageSamplerCount);

    static int maxAccelerationStructureCount = 512;
    static CVarInt maxAccelerationStructureCountCVar(
        "vk.MaxDescriptorCount.AccelerationStructure", maxAccelerationStructureCount);

    static std::vector<vk::DescriptorPoolSize> GetDescriptorPoolSizes()
    {
        return {
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, maxUniformBufferCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, maxStorageBufferCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, maxStorageImageCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, maxCombinedImageSamplerCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eAccelerationStructureKHR, maxAccelerationStructureCount),
        };
    }

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

std::unique_ptr<DescriptorManager> DescriptorManager::Create()
{
    constexpr vk::DescriptorPoolCreateFlags createFlags
            = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
            | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind; // TODO implement separate pool

    const std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = Details::GetDescriptorPoolSizes();

    const vk::DescriptorPoolCreateInfo createInfo(createFlags, Details::maxDescriptorSetCount, descriptorPoolSizes);

    const auto [result, pool] = VulkanContext::device->Get().createDescriptorPool(createInfo);
    Assert(result == vk::Result::eSuccess);

    return std::unique_ptr<DescriptorManager>(new DescriptorManager(pool));
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
