#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace Details
{
    uint32_t ComputeSliceCount(const std::map<DescriptorKey, std::vector<DescriptorData>>& dataMap)
    {
        const auto it = std::ranges::max_element(dataMap, [](const auto& a, const auto& b)
            {
                return a.second.size() < b.second.size();
            });

        return static_cast<uint32_t>(it->second.size());
    }

    vk::DescriptorSet AllocateDescriptorSet(vk::DescriptorSetLayout layout)
    {
        return VulkanContext::descriptorPool->AllocateDescriptorSets({ layout }).front();
    }
}

DescriptorProvider::DescriptorProvider(const DescriptorsReflection& reflection_,
        const std::vector<vk::DescriptorSetLayout>& layouts_)
    : reflection(reflection_)
    , layouts(layouts_)
{}

DescriptorProvider::~DescriptorProvider()
{
    FreeDescriptors();
}

void DescriptorProvider::PushGlobalData(const std::string& name, const DescriptorSources& sources)
{
    const auto it = reflection.find(name);

    Assert(it != reflection.end());

    dataMap[it->second.key] = { DescriptorHelpers::GetData(it->second.type, sources) };
}

void DescriptorProvider::PushGlobalData(const std::string& name, const DescriptorSource& source)
{
    const auto it = reflection.find(name);

    Assert(it != reflection.end());

    dataMap[it->second.key] = { DescriptorHelpers::GetData(it->second.type, source) };
}

void DescriptorProvider::PushGlobalData(const std::string& name, const DescriptorData& data)
{
    const auto it = reflection.find(name);

    Assert(it != reflection.end());
    Assert(it->second.type == data.type);

    dataMap[it->second.key] = { data };
}

void DescriptorProvider::PushSliceData(const std::string& name, const DescriptorSources& sources)
{
    const auto it = reflection.find(name);

    Assert(it != reflection.end());

    dataMap[it->second.key].push_back(DescriptorHelpers::GetData(it->second.type, sources));
}

void DescriptorProvider::PushSliceData(const std::string& name, const DescriptorSource& source)
{
    const auto it = reflection.find(name);

    Assert(it != reflection.end());

    dataMap[it->second.key].push_back(DescriptorHelpers::GetData(it->second.type, source));
}

void DescriptorProvider::PushSliceData(const std::string& name, const DescriptorData& data)
{
    const auto it = reflection.find(name);

    Assert(it != reflection.end());
    Assert(it->second.type == data.type);

    dataMap[it->second.key].push_back(data);
}

void DescriptorProvider::FlushData()
{
    if (dataMap.empty())
    {
        return;
    }

    if (descriptors.empty())
    {
        AllocateDescriptors();
    }

    UpdateDescriptors();

    dataMap.clear();
}

void DescriptorProvider::Clear()
{
    FreeDescriptors();
}

const DescriptorSlice& DescriptorProvider::GetDescriptorSlice(uint32_t sliceIndex) const
{
    Assert(sliceIndex < GetSliceCount());

    return descriptorSlices[sliceIndex];
}

uint32_t DescriptorProvider::GetSliceCount() const
{
    return static_cast<uint32_t>(descriptorSlices.size());
}

uint32_t DescriptorProvider::GetSetCount() const
{
    Assert(!descriptorSlices.empty());

    return static_cast<uint32_t>(descriptorSlices.front().size());
}

void DescriptorProvider::AllocateDescriptors()
{
    const uint32_t sliceCount = Details::ComputeSliceCount(dataMap);

    std::vector<DescriptorSetRate> rates(layouts.size(), DescriptorSetRate::eGlobal);

    for (const auto& [key, data] : dataMap)
    {
        if (data.size() == sliceCount)
        {
            rates[key.set] = DescriptorSetRate::ePerSlice;
        }
        else
        {
            Assert(data.size() == 1);
            Assert(rates[key.set] == DescriptorSetRate::eGlobal);
        }
    }

    descriptorSlices.resize(sliceCount);

    for (auto& descriptorSlice : descriptorSlices)
    {
        descriptorSlice.resize(layouts.size());
    }

    descriptors.reserve(layouts.size());

    for (size_t i = 0; i < rates.size(); ++i)
    {
        const DescriptorSetRate rate = rates[i];

        if (rate == DescriptorSetRate::ePerSlice)
        {
            for (auto& descriptorSlice : descriptorSlices)
            {
                descriptorSlice[i] = Details::AllocateDescriptorSet(layouts[i]);

                descriptors.push_back(descriptorSlice[i]);
            }
        }
        else
        {
            Assert(rate == DescriptorSetRate::eGlobal);

            const vk::DescriptorSet descriptorSet = Details::AllocateDescriptorSet(layouts[i]);

            for (auto& descriptorSlice : descriptorSlices)
            {
                descriptorSlice[i] = descriptorSet;
            }

            descriptors.push_back(descriptorSet);
        }
    }
}

void DescriptorProvider::UpdateDescriptors()
{
    std::vector<vk::WriteDescriptorSet> writes;
    writes.reserve(dataMap.size());

    for (const auto& [key, data] : dataMap)
    {
        Assert(data.size() == 1 || data.size() == descriptorSlices.size());

        for (size_t i = 0; i < data.size(); ++i)
        {
            Assert(key.set < static_cast<uint32_t>(descriptorSlices[i].size()));

            vk::WriteDescriptorSet write(descriptorSlices[i][key.set], key.binding, 0, 0, data[i].type);

            if (DescriptorHelpers::WriteDescriptorData(write, data[i]))
            {
                writes.push_back(write);
            }
        }
    }

    VulkanContext::descriptorPool->UpdateDescriptorSet(writes);
}

void DescriptorProvider::FreeDescriptors()
{
    if (!descriptors.empty())
    {
        VulkanContext::descriptorPool->FreeDescriptorSets(descriptors);

        descriptorSlices.clear();

        descriptors.clear();
    }
}
