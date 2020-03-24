#pragma once

struct DescriptorDescription
{
    vk::DescriptorType type;
    vk::ShaderStageFlags stageFlags;

    bool operator ==(const DescriptorDescription &other) const;
};

using DescriptorSetDescription = std::vector<DescriptorDescription>;

using DescriptorInfo = std::variant<vk::DescriptorImageInfo, vk::DescriptorBufferInfo,
    vk::BufferView, vk::WriteDescriptorSetAccelerationStructureNV>;

struct DescriptorData
{
    vk::DescriptorType type;
    DescriptorInfo info;
};

using DescriptorSetData = std::vector<DescriptorData>;

class DescriptorPool
{
public:
    static std::unique_ptr<DescriptorPool> Create(uint32_t maxSetCount,
            const std::vector<vk::DescriptorPoolSize> &poolSizes);
    ~DescriptorPool();

    vk::DescriptorSetLayout CreateDescriptorSetLayout(const DescriptorSetDescription &description);

    void DestroyDescriptorSetLayout(vk::DescriptorSetLayout layout);

    std::vector<vk::DescriptorSet> AllocateDescriptorSets(
            const std::vector<vk::DescriptorSetLayout> &layouts) const;

    std::vector<vk::DescriptorSet> AllocateDescriptorSets(
            vk::DescriptorSetLayout layout, uint32_t count) const;

    vk::DescriptorSet AllocateDescriptorSet(vk::DescriptorSetLayout layout) const;

    void UpdateDescriptorSet(vk::DescriptorSet descriptorSet,
            const DescriptorSetData &descriptorSetData, uint32_t bindingOffset) const;

private:
    struct LayoutCacheEntry
    {
        DescriptorSetDescription description;
        vk::DescriptorSetLayout layout;
    };

    vk::DescriptorPool descriptorPool;

    std::list<LayoutCacheEntry> layoutCache;

    DescriptorPool(vk::DescriptorPool descriptorPool_);
};
