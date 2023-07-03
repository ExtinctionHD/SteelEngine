#pragma once

#include "Engine/Render/Vulkan/Resources/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"

using DescriptorSlice = std::vector<vk::DescriptorSet>;

class DescriptorProvider
{
public:
    DescriptorProvider(const DescriptorsReflection& reflection_,
            const std::vector<vk::DescriptorSetLayout>& layouts_);

    ~DescriptorProvider();

    void PushGlobalData(const std::string& name, const DescriptorSources& sources);
    void PushGlobalData(const std::string& name, const DescriptorSource& source);
    void PushGlobalData(const std::string& name, const DescriptorData& data);

    void PushSliceData(const std::string& name, const DescriptorSources& sources);
    void PushSliceData(const std::string& name, const DescriptorSource& source);
    void PushSliceData(const std::string& name, const DescriptorData& data);

    void FlushData();

    void Clear();

    const DescriptorSlice& GetDescriptorSlice(uint32_t sliceIndex = 0) const;

    uint32_t GetSliceCount() const;

    uint32_t GetSetCount() const;

private:
    DescriptorsReflection reflection;

    std::vector<vk::DescriptorSetLayout> layouts;
    std::map<DescriptorKey, std::vector<DescriptorData>> dataMap;

    std::vector<DescriptorSlice> descriptorSlices;
    std::vector<vk::DescriptorSet> descriptors;

    void AllocateDescriptors();

    void UpdateDescriptors();

    void FreeDescriptors();
};
