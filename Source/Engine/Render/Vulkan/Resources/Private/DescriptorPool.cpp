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
                    properties[i].type, 1, properties[i].stageFlags);
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
    for (auto &[properties, layout] : layoutsCache)
    {
        device->Get().destroyDescriptorSetLayout(layout);
    }
    device->Get().destroyDescriptorPool(descriptorPool);
}

vk::DescriptorSetLayout DescriptorPool::CreateDescriptorSetLayout(DescriptorSetProperties &properties)
{
    const auto it = std::find_if(layoutsCache.begin(), layoutsCache.end(), [&properties](const auto &entry)
        {
            return entry.properties == properties;
        });

    if (it != layoutsCache.end())
    {
        return it->layout;
    }

    const std::vector<vk::DescriptorSetLayoutBinding> bindings = SDescriptorPool::ObtainBindings(properties);
    const vk::DescriptorSetLayoutCreateInfo createInfo({}, static_cast<uint32_t>(bindings.size()), bindings.data());

    const auto [result, layout] = device->Get().createDescriptorSetLayout(createInfo);
    Assert(result == vk::Result::eSuccess);

    layoutsCache.push_back({ properties, layout });

    return layout;
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

vk::DescriptorSet DescriptorPool::AllocateDescriptorSet(vk::DescriptorSetLayout layout) const
{
    return AllocateDescriptorSets({ layout }).front();
}

void DescriptorPool::UpdateDescriptorSet(vk::DescriptorSet descriptorSet,
        const std::vector<DescriptorData> &descriptorSetData) const
{
    std::vector<vk::WriteDescriptorSet> descriptorWrites(descriptorSetData.size());

    for (uint32_t i = 0; i < descriptorWrites.size(); ++i)
    {
        const vk::DescriptorType type = descriptorSetData[i].type;
        const DescriptorInfo info = descriptorSetData[i].info;

        vk::DescriptorBufferInfo bufferInfo; 
        vk::DescriptorImageInfo imageInfo;
        vk::BufferView bufferView;

        switch (type)
        {
        case vk::DescriptorType::eSampler:
        case vk::DescriptorType::eCombinedImageSampler:
        case vk::DescriptorType::eSampledImage:
        case vk::DescriptorType::eStorageImage:
        case vk::DescriptorType::eInputAttachment:
            bufferInfo = std::get<vk::DescriptorBufferInfo>(info);
            break;

        case vk::DescriptorType::eUniformBuffer:
        case vk::DescriptorType::eStorageBuffer:
        case vk::DescriptorType::eUniformBufferDynamic:
        case vk::DescriptorType::eStorageBufferDynamic:
            imageInfo = std::get<vk::DescriptorImageInfo>(info);
            break;

        case vk::DescriptorType::eUniformTexelBuffer:
        case vk::DescriptorType::eStorageTexelBuffer:
            bufferView = std::get<vk::BufferView>(info);
            break;

        case vk::DescriptorType::eInlineUniformBlockEXT:
        case vk::DescriptorType::eAccelerationStructureNV:
            Assert(false);
        }

        descriptorWrites[i] = vk::WriteDescriptorSet(descriptorSet, i, 0, 1, type, &imageInfo, &bufferInfo, &bufferView);
    }
    
    device->Get().updateDescriptorSets(descriptorWrites, {});
}

bool DescriptorPool::LayoutsCacheEntry::operator==(const LayoutsCacheEntry &other) const
{
    return properties == other.properties && layout == other.layout;
}

