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

        for (uint32_t i = 0; i < descriptorSetDescription.size(); ++i)
        {
            if (descriptorSetDescription[i].count == 0)
            {
                continue;
            }

            const vk::DescriptorSetLayoutBinding binding(i,
                    descriptorSetDescription[i].type, 
                    descriptorSetDescription[i].count, 
                    descriptorSetDescription[i].stageFlags);

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
            if (descriptorDescription.count == 0)
            {
                continue;
            }

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

void DescriptorPool::FreeDescriptorSets(const std::vector<vk::DescriptorSet>& descriptorSets) const
{
    VulkanContext::device->Get().freeDescriptorSets(descriptorPool, descriptorSets);
}

void DescriptorPool::UpdateDescriptorSet(vk::DescriptorSet descriptorSet,
        const DescriptorSetData& descriptorSetData, uint32_t bindingOffset) const
{
    std::vector<vk::WriteDescriptorSet> descriptorWrites;

    for (uint32_t i = 0; i < descriptorSetData.size(); ++i)
    {
        const auto& [type, descriptorInfo] = descriptorSetData[i];

        if (std::holds_alternative<std::monostate>(descriptorInfo))
        {
            continue;
        }

        vk::WriteDescriptorSet descriptorWrite(descriptorSet, bindingOffset + i, 0, 0, type);

        switch (type)
        {
        case vk::DescriptorType::eSampler:
        case vk::DescriptorType::eCombinedImageSampler:
        case vk::DescriptorType::eSampledImage:
        case vk::DescriptorType::eStorageImage:
        case vk::DescriptorType::eInputAttachment:
        {
            const ImageInfo& imageInfo = std::get<ImageInfo>(descriptorInfo);

            if (imageInfo.empty())
            {
                continue;
            }

            descriptorWrite.descriptorCount = static_cast<uint32_t>(imageInfo.size());
            descriptorWrite.pImageInfo = imageInfo.data();
            break;
        }
        case vk::DescriptorType::eUniformBuffer:
        case vk::DescriptorType::eStorageBuffer:
        case vk::DescriptorType::eUniformBufferDynamic:
        case vk::DescriptorType::eStorageBufferDynamic:
        {
            const BufferInfo& bufferInfo = std::get<BufferInfo>(descriptorInfo);

            if (bufferInfo.empty())
            {
                continue;
            }

            descriptorWrite.descriptorCount = static_cast<uint32_t>(bufferInfo.size());
            descriptorWrite.pBufferInfo = bufferInfo.data();
            break;
        }
        case vk::DescriptorType::eUniformTexelBuffer:
        case vk::DescriptorType::eStorageTexelBuffer:
        {
            const BufferViews& bufferViews = std::get<BufferViews>(descriptorInfo);

            if (bufferViews.empty())
            {
                continue;
            }

            descriptorWrite.descriptorCount = static_cast<uint32_t>(bufferViews.size());
            descriptorWrite.pTexelBufferView = bufferViews.data();
            break;
        }
        case vk::DescriptorType::eAccelerationStructureKHR:
        {
            const AccelerationStructureInfo& accelerationStructureInfo
                    = std::get<AccelerationStructureInfo>(descriptorInfo);

            if (accelerationStructureInfo.accelerationStructureCount == 0)
            {
                continue;
            }

            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pNext = &accelerationStructureInfo;
            break;
        }
        case vk::DescriptorType::eInlineUniformBlockEXT:
        default:
            Assert(false);
            break;
        }

        descriptorWrites.push_back(descriptorWrite);
    }

    VulkanContext::device->Get().updateDescriptorSets(descriptorWrites, {});
}
