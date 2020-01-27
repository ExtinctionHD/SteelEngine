#include "Engine/Render/Vulkan/DescriptorPool.hpp"

#include "Utils/Assert.hpp"

namespace SDescriptorPool
{
    std::vector<vk::DescriptorSetLayoutBinding> ObtainBindings(const std::vector<DescriptorDescription> &description)
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
        const std::set<vk::DescriptorType> &descriptorTypes)
{
    constexpr uint32_t descriptorCount = 1024;
    constexpr uint32_t maxSets = 1024;

    std::vector<vk::DescriptorPoolSize> poolSizes;
    poolSizes.reserve(descriptorTypes.size());

    for (const auto &type : descriptorTypes)
    {
        poolSizes.emplace_back(type, descriptorCount);
    }

    const vk::DescriptorPoolCreateInfo createInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, maxSets,
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

    const std::vector<vk::DescriptorSetLayoutBinding> bindings = SDescriptorPool::ObtainBindings(description);
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

vk::DescriptorSet DescriptorPool::AllocateDescriptorSet(vk::DescriptorSetLayout layout) const
{
    return AllocateDescriptorSets({ layout }).front();
}

void DescriptorPool::UpdateDescriptorSet(vk::DescriptorSet descriptorSet,
        const DescriptorSetData &descriptorSetData, uint32_t bindingOffset)
{
    descriptorWrites.reserve(descriptorWrites.size() + descriptorSetData.size());

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

        descriptorWrites.emplace_back(descriptorSet, bindingOffset + i,
                0, 1, type, &imageInfo, &bufferInfo, &bufferView);
    }
}

void DescriptorPool::PerformUpdate()
{
    if (!descriptorWrites.empty())
    {
        device->Get().updateDescriptorSets(descriptorWrites, {});
        descriptorWrites.clear();
    }
}
