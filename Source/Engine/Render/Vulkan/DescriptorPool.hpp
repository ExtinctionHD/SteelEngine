#pragma once

struct DescriptorDescription
{
    vk::DescriptorType type;
    uint32_t count;

    vk::ShaderStageFlags stageFlags;
    vk::DescriptorBindingFlags bindingFlags;

    bool operator==(const DescriptorDescription &other) const;
};

using DescriptorSetDescription = std::vector<DescriptorDescription>;

using ImagesInfo = std::vector<vk::DescriptorImageInfo>;
using BuffersInfo = std::vector<vk::DescriptorBufferInfo>;
using BufferViews = std::vector<vk::BufferView>;
using AccelerationStructuresInfo = vk::WriteDescriptorSetAccelerationStructureNV;

using DescriptorsInfo = std::variant<ImagesInfo, BuffersInfo, BufferViews, AccelerationStructuresInfo>;

struct DescriptorData
{
    vk::DescriptorType type;
    DescriptorsInfo descriptorsInfo;
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

    std::vector<vk::DescriptorSet> AllocateDescriptorSets(const std::vector<vk::DescriptorSetLayout> &layouts) const;

    std::vector<vk::DescriptorSet> AllocateDescriptorSets(const std::vector<vk::DescriptorSetLayout> &layouts,
            const std::vector<uint32_t> &descriptorCounts) const;

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
