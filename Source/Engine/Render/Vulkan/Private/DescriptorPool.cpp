#include <experimental/unordered_map>

#include "Engine/Render/Vulkan/DescriptorPool.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"

namespace SDescriptorPool
{
    auto GetBindings(const DescriptorSetDescription &description)
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings(description.size());
        std::vector<vk::DescriptorBindingFlags> bindingFlags(description.size());

        for (uint32_t i = 0; i < description.size(); ++i)
        {
            bindings[i] = vk::DescriptorSetLayoutBinding(i,
                    description[i].type, description[i].count, description[i].stageFlags);
        }

        return std::make_pair(bindings, bindingFlags);
    }
}

std::unique_ptr<DescriptorPool> DescriptorPool::Create(uint32_t maxSetCount,
        const std::vector<vk::DescriptorPoolSize> &poolSizes)
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

vk::DescriptorSetLayout DescriptorPool::CreateDescriptorSetLayout(const DescriptorSetDescription &description) const
{
    const auto [bindings, bindingFlags] = SDescriptorPool::GetBindings(description);

    const vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo(
            static_cast<uint32_t>(bindingFlags.size()), bindingFlags.data());

    vk::StructureChain<vk::DescriptorSetLayoutCreateInfo, vk::DescriptorSetLayoutBindingFlagsCreateInfo> structures(
            vk::DescriptorSetLayoutCreateInfo({}, static_cast<uint32_t>(bindings.size()), bindings.data()),
            bindingFlagsCreateInfo);

    const vk::DescriptorSetLayoutCreateInfo &createInfo = structures.get<vk::DescriptorSetLayoutCreateInfo>();

    const auto [result, layout] = VulkanContext::device->Get().createDescriptorSetLayout(createInfo);
    Assert(result == vk::Result::eSuccess);

    return layout;
}

void DescriptorPool::DestroyDescriptorSetLayout(vk::DescriptorSetLayout layout)
{
    VulkanContext::device->Get().destroyDescriptorSetLayout(layout);
}

std::vector<vk::DescriptorSet> DescriptorPool::AllocateDescriptorSets(
        const std::vector<vk::DescriptorSetLayout> &layouts) const
{
    const vk::DescriptorSetAllocateInfo allocateInfo(descriptorPool,
            static_cast<uint32_t>(layouts.size()), layouts.data());

    const auto [result, allocatedSets] = VulkanContext::device->Get().allocateDescriptorSets(allocateInfo);
    Assert(result == vk::Result::eSuccess);

    return allocatedSets;
}

std::vector<vk::DescriptorSet> DescriptorPool::AllocateDescriptorSets(
        const std::vector<vk::DescriptorSetLayout> &layouts,
        const std::vector<uint32_t> &descriptorCounts) const
{
    const vk::DescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountAllocateInfo(
            static_cast<uint32_t>(descriptorCounts.size()), descriptorCounts.data());

    vk::StructureChain<vk::DescriptorSetAllocateInfo, vk::DescriptorSetVariableDescriptorCountAllocateInfo> structures(
            vk::DescriptorSetAllocateInfo(descriptorPool, static_cast<uint32_t>(layouts.size()), layouts.data()),
            variableDescriptorCountAllocateInfo);

    const vk::DescriptorSetAllocateInfo &allocateInfo = structures.get<vk::DescriptorSetAllocateInfo>();

    const auto [result, allocatedSets] = VulkanContext::device->Get().allocateDescriptorSets(allocateInfo);
    Assert(result == vk::Result::eSuccess);

    return allocatedSets;
}

void DescriptorPool::FreeDescriptorSets(const std::vector<vk::DescriptorSet> &descriptorSets) const
{
    const vk::Result result = VulkanContext::device->Get().freeDescriptorSets(descriptorPool, descriptorSets);
    Assert(result == vk::Result::eSuccess);
}

void DescriptorPool::UpdateDescriptorSet(vk::DescriptorSet descriptorSet,
        const DescriptorSetData &descriptorSetData, uint32_t bindingOffset) const
{
    std::vector<vk::WriteDescriptorSet> descriptorWrites;

    for (uint32_t i = 0; i < descriptorSetData.size(); ++i)
    {
        const auto &[type, descriptorInfo] = descriptorSetData[i];

        vk::WriteDescriptorSet descriptorWrite(descriptorSet, bindingOffset + i, 0, 0, type);

        switch (type)
        {
        case vk::DescriptorType::eSampler:
        case vk::DescriptorType::eCombinedImageSampler:
        case vk::DescriptorType::eSampledImage:
        case vk::DescriptorType::eStorageImage:
        case vk::DescriptorType::eInputAttachment:
        {
            const ImageInfo &imageInfo = std::get<ImageInfo>(descriptorInfo);
            descriptorWrite.descriptorCount = static_cast<uint32_t>(imageInfo.size());
            descriptorWrite.pImageInfo = imageInfo.data();
            break;
        }
        case vk::DescriptorType::eUniformBuffer:
        case vk::DescriptorType::eStorageBuffer:
        case vk::DescriptorType::eUniformBufferDynamic:
        case vk::DescriptorType::eStorageBufferDynamic:
        {
            const BufferInfo &bufferInfo = std::get<BufferInfo>(descriptorInfo);
            descriptorWrite.descriptorCount = static_cast<uint32_t>(bufferInfo.size());
            descriptorWrite.pBufferInfo = bufferInfo.data();
            break;
        }
        case vk::DescriptorType::eUniformTexelBuffer:
        case vk::DescriptorType::eStorageTexelBuffer:
        {
            const BufferViews &bufferViews = std::get<BufferViews>(descriptorInfo);
            descriptorWrite.descriptorCount = static_cast<uint32_t>(bufferViews.size());
            descriptorWrite.pTexelBufferView = bufferViews.data();
            break;
        }
        case vk::DescriptorType::eAccelerationStructureNV:
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pNext = &std::get<AccelerationStructureInfo>(descriptorInfo);
            break;

        case vk::DescriptorType::eInlineUniformBlockEXT:
            Assert(false);
        }

        descriptorWrites.push_back(descriptorWrite);
    }

    VulkanContext::device->Get().updateDescriptorSets(descriptorWrites, {});
}
