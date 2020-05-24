#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

DescriptorData DescriptorHelpers::GetData(vk::Sampler sampler, vk::ImageView view)
{
    return DescriptorData{
        vk::DescriptorType::eCombinedImageSampler,
        ImageInfo{
            vk::DescriptorImageInfo(sampler, view, vk::ImageLayout::eShaderReadOnlyOptimal)
        }
    };
}

DescriptorData DescriptorHelpers::GetData(vk::ImageView view)
{
    return DescriptorData{
        vk::DescriptorType::eStorageImage,
        ImageInfo{
            vk::DescriptorImageInfo(nullptr, view, vk::ImageLayout::eGeneral)
        }
    };
}

DescriptorData DescriptorHelpers::GetData(vk::Buffer buffer)
{
    return DescriptorData{
        vk::DescriptorType::eUniformBuffer,
        BufferInfo{
            vk::DescriptorBufferInfo(buffer, 0, VK_WHOLE_SIZE)
        }
    };
}

DescriptorData DescriptorHelpers::GetData(const vk::AccelerationStructureNV &accelerationStructure)
{
    return DescriptorData{
        vk::DescriptorType::eAccelerationStructureNV,
        AccelerationStructureInfo(1, &accelerationStructure)
    };
}

DescriptorSet DescriptorHelpers::CreateDescriptorSet(const DescriptorSetDescription &description,
        const DescriptorSetData &descriptorSetData)
{
    const vk::DescriptorSetLayout layout = VulkanContext::descriptorPool->CreateDescriptorSetLayout(description);
    const vk::DescriptorSet value = VulkanContext::descriptorPool->AllocateDescriptorSets({ layout }).front();

    VulkanContext::descriptorPool->UpdateDescriptorSet(value, descriptorSetData, 0);

    return DescriptorSet{ layout, value };
}

MultiDescriptorSet DescriptorHelpers::CreateMultiDescriptorSet(const DescriptorSetDescription &description,
        const std::vector<DescriptorSetData> &multiDescriptorSetData)
{
    const vk::DescriptorSetLayout layout = VulkanContext::descriptorPool->CreateDescriptorSetLayout(description);
    const std::vector<vk::DescriptorSet> values = VulkanContext::descriptorPool->AllocateDescriptorSets(
            Repeat(layout, multiDescriptorSetData.size()));

    for (size_t i = 0; i < multiDescriptorSetData.size(); ++i)
    {
        VulkanContext::descriptorPool->UpdateDescriptorSet(values[i], multiDescriptorSetData[i], 0);
    }

    return MultiDescriptorSet{ layout, values };
}

void DescriptorHelpers::DestroyDescriptorSet(const DescriptorSet &descriptorSet)
{
    VulkanContext::descriptorPool->FreeDescriptorSets({ descriptorSet.value });
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(descriptorSet.layout);
}

void DescriptorHelpers::DestroyMultiDescriptorSet(const MultiDescriptorSet &multiDescriptorSet)
{
    VulkanContext::descriptorPool->FreeDescriptorSets(multiDescriptorSet.values);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(multiDescriptorSet.layout);
}
