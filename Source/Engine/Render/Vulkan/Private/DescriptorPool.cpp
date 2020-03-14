#include "Engine/Render/Vulkan/DescriptorPool.hpp"

#include "Utils/Assert.hpp"

namespace SDescriptorPool
{
    std::vector<vk::DescriptorSetLayoutBinding> GetBindings(const std::vector<DescriptorDescription> &description)
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings(description.size());

        for (uint32_t i = 0; i < description.size(); ++i)
        {
            bindings[i] = vk::DescriptorSetLayoutBinding(i,
                    description[i].type, 1, description[i].stageFlags);
        }

        return bindings;
    }
}

bool DescriptorDescription::operator==(const DescriptorDescription &other) const
{
    return type == other.type && stageFlags == other.stageFlags;
}

std::unique_ptr<DescriptorPool> DescriptorPool::Create(std::shared_ptr<Device> device,
        const std::vector<vk::DescriptorPoolSize> &poolSizes, uint32_t maxSetCount)
{
    const vk::DescriptorPoolCreateInfo createInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, maxSetCount,
            static_cast<uint32_t>(poolSizes.size()), poolSizes.data());

    const auto [result, descriptorPool] = device->Get().createDescriptorPool(createInfo);
    Assert(result == vk::Result::eSuccess);

    return std::unique_ptr<DescriptorPool>(new DescriptorPool(device, descriptorPool));
}

DescriptorPool::DescriptorPool(std::shared_ptr<Device> device_, vk::DescriptorPool descriptorPool_)
    : device(device_)
    , descriptorPool(descriptorPool_)
{}

DescriptorPool::~DescriptorPool()
{
    for (auto &[description, layout] : layoutCache)
    {
        device->Get().destroyDescriptorSetLayout(layout);
    }
    device->Get().destroyDescriptorPool(descriptorPool);
}

vk::DescriptorSetLayout DescriptorPool::CreateDescriptorSetLayout(const DescriptorSetDescription &description)
{
    const auto it = std::find_if(layoutCache.begin(), layoutCache.end(), [&description](const auto &entry)
        {
            return entry.description == description;
        });

    if (it != layoutCache.end())
    {
        return it->layout;
    }

    const std::vector<vk::DescriptorSetLayoutBinding> bindings = SDescriptorPool::GetBindings(description);
    const vk::DescriptorSetLayoutCreateInfo createInfo({}, static_cast<uint32_t>(bindings.size()), bindings.data());

    const auto [result, layout] = device->Get().createDescriptorSetLayout(createInfo);
    Assert(result == vk::Result::eSuccess);

    layoutCache.push_back({ description, layout });

    return layout;
}

void DescriptorPool::DestroyDescriptorSetLayout(vk::DescriptorSetLayout layout)
{
    device->Get().destroyDescriptorSetLayout(layout);

    layoutCache.remove_if([&layout](const auto &entry)
        {
            return entry.layout == layout;
        });
}

std::vector<vk::DescriptorSet> DescriptorPool::AllocateDescriptorSets(
        const std::vector<vk::DescriptorSetLayout> &layouts) const
{
    const vk::DescriptorSetAllocateInfo allocateInfo(descriptorPool,
            static_cast<uint32_t>(layouts.size()), layouts.data());

    const auto [result, allocatedSets] = device->Get().allocateDescriptorSets(allocateInfo);
    Assert(result == vk::Result::eSuccess);

    return allocatedSets;
}

std::vector<vk::DescriptorSet> DescriptorPool::AllocateDescriptorSets(
        vk::DescriptorSetLayout layout, uint32_t count) const
{
    const std::vector<vk::DescriptorSetLayout> layouts(count, layout);

    return AllocateDescriptorSets(layouts);
}

vk::DescriptorSet DescriptorPool::AllocateDescriptorSet(vk::DescriptorSetLayout layout) const
{
    return AllocateDescriptorSets(layout, 1).front();
}

void DescriptorPool::UpdateDescriptorSet(vk::DescriptorSet descriptorSet,
        const DescriptorSetData &descriptorSetData, uint32_t bindingOffset) const
{
    std::vector<vk::WriteDescriptorSet> descriptorWrites;

    std::list<vk::DescriptorImageInfo> imageInfos;
    std::list<vk::DescriptorBufferInfo> bufferInfos;
    std::list<vk::BufferView> bufferViews;
    std::list<vk::WriteDescriptorSetAccelerationStructureNV> accelerationStructureInfos;

    for (uint32_t i = 0; i < descriptorSetData.size(); ++i)
    {
        const auto &[type, info] = descriptorSetData[i];

        vk::WriteDescriptorSet descriptorWrite(descriptorSet, bindingOffset + i, 0, 1, type);

        switch (type)
        {
        case vk::DescriptorType::eSampler:
        case vk::DescriptorType::eCombinedImageSampler:
        case vk::DescriptorType::eSampledImage:
        case vk::DescriptorType::eStorageImage:
        case vk::DescriptorType::eInputAttachment:
            imageInfos.push_back(std::get<vk::DescriptorImageInfo>(info));
            descriptorWrite.pImageInfo = &imageInfos.back();
            break;

        case vk::DescriptorType::eUniformBuffer:
        case vk::DescriptorType::eStorageBuffer:
        case vk::DescriptorType::eUniformBufferDynamic:
        case vk::DescriptorType::eStorageBufferDynamic:
            bufferInfos.push_back(std::get<vk::DescriptorBufferInfo>(info));
            descriptorWrite.pBufferInfo = &bufferInfos.back();
            break;

        case vk::DescriptorType::eUniformTexelBuffer:
        case vk::DescriptorType::eStorageTexelBuffer:
            bufferViews.push_back(std::get<vk::BufferView>(info));
            descriptorWrite.pTexelBufferView = &bufferViews.back();
            break;

        case vk::DescriptorType::eAccelerationStructureNV:
            accelerationStructureInfos.push_back(std::get<vk::WriteDescriptorSetAccelerationStructureNV>(info));
            descriptorWrite.pNext = &accelerationStructureInfos.back();
            break;

        case vk::DescriptorType::eInlineUniformBlockEXT:
            Assert(false);
        }

        descriptorWrites.push_back(descriptorWrite);
    }

    device->Get().updateDescriptorSets(descriptorWrites, {});
}
