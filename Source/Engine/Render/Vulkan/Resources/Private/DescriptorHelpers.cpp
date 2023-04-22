#include "Engine/Render/Vulkan/Resources/DescriptorHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

DescriptorData DescriptorHelpers::GetData(vk::Sampler sampler, vk::ImageView view)
{
    if (!sampler)
    {
        return DescriptorData{
            vk::DescriptorType::eCombinedImageSampler,
            ImageInfo{}
        };
    }

    return DescriptorData{
        vk::DescriptorType::eCombinedImageSampler,
        ImageInfo{
            vk::DescriptorImageInfo(sampler, view, vk::ImageLayout::eShaderReadOnlyOptimal)
        }
    };
}

DescriptorData DescriptorHelpers::GetData(vk::Sampler sampler, const std::vector<vk::ImageView>& views)
{
    ImageInfo imageInfo;
    imageInfo.reserve(views.size());

    for (const auto& view : views)
    {
        imageInfo.emplace_back(sampler, view, vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    return DescriptorData{ vk::DescriptorType::eCombinedImageSampler, imageInfo };
}

DescriptorData DescriptorHelpers::GetData(const std::vector<SampledTexture>& textures)
{
    ImageInfo imageInfo;
    imageInfo.reserve(textures.size());

    for (const SampledTexture& texture : textures)
    {
        const vk::DescriptorImageInfo info{
            texture.sampler, texture.view,
            vk::ImageLayout::eShaderReadOnlyOptimal
        };

        imageInfo.push_back(info);
    }

    return DescriptorData{ vk::DescriptorType::eCombinedImageSampler, imageInfo };
}

DescriptorData DescriptorHelpers::GetData(vk::Buffer buffer)
{
    if (!buffer)
    {
        return DescriptorData{
            vk::DescriptorType::eUniformBuffer,
            BufferInfo{}
        };
    }

    return DescriptorData{
        vk::DescriptorType::eUniformBuffer,
        BufferInfo{
            vk::DescriptorBufferInfo(buffer, 0, VK_WHOLE_SIZE)
        }
    };
}

DescriptorData DescriptorHelpers::GetStorageData(vk::ImageView view)
{
    if (!view)
    {
        return DescriptorData{
            vk::DescriptorType::eStorageImage,
            ImageInfo{}
        };
    }

    return DescriptorData{
        vk::DescriptorType::eStorageImage,
        ImageInfo{
            vk::DescriptorImageInfo(nullptr, view, vk::ImageLayout::eGeneral)
        }
    };
}

DescriptorData DescriptorHelpers::GetStorageData(const std::vector<vk::ImageView>& views)
{
    ImageInfo imageInfo;
    imageInfo.reserve(views.size());

    for (const auto& view : views)
    {
        imageInfo.emplace_back(nullptr, view, vk::ImageLayout::eGeneral);
    }

    return DescriptorData{ vk::DescriptorType::eStorageImage, imageInfo };
}

DescriptorData DescriptorHelpers::GetStorageData(vk::Buffer buffer)
{
    if (!buffer)
    {
        return DescriptorData{
            vk::DescriptorType::eStorageBuffer,
            BufferInfo{}
        };
    }

    return DescriptorData{
        vk::DescriptorType::eStorageBuffer,
        BufferInfo{
            vk::DescriptorBufferInfo(buffer, 0, VK_WHOLE_SIZE)
        }
    };
}

DescriptorData DescriptorHelpers::GetStorageData(const std::vector<vk::Buffer>& buffers)
{
    BufferInfo bufferInfo;
    bufferInfo.reserve(buffers.size());

    for (const auto& buffer : buffers)
    {
        bufferInfo.emplace_back(buffer, 0, VK_WHOLE_SIZE);
    }

    return DescriptorData{ vk::DescriptorType::eStorageBuffer, bufferInfo };
}

DescriptorData DescriptorHelpers::GetData(const vk::AccelerationStructureKHR& accelerationStructure)
{
    if (!accelerationStructure)
    {
        return DescriptorData{
            vk::DescriptorType::eAccelerationStructureKHR,
            AccelerationStructureInfo()
        };
    }

    return DescriptorData{
        vk::DescriptorType::eAccelerationStructureKHR,
        AccelerationStructureInfo(1, &accelerationStructure)
    };
}

DescriptorSet DescriptorHelpers::CreateDescriptorSet(const DescriptorSetDescription& description,
        const DescriptorSetData& descriptorSetData)
{
    Assert(!description.empty() && description.size() == descriptorSetData.size());

    const vk::DescriptorSetLayout layout = VulkanContext::descriptorPool->CreateDescriptorSetLayout(description);
    const vk::DescriptorSet value = VulkanContext::descriptorPool->AllocateDescriptorSets({ layout }).front();

    VulkanContext::descriptorPool->UpdateDescriptorSet(value, descriptorSetData, 0);

    return DescriptorSet{ layout, value };
}

MultiDescriptorSet DescriptorHelpers::CreateMultiDescriptorSet(const DescriptorSetDescription& description,
        const std::vector<DescriptorSetData>& multiDescriptorSetData)
{
    const auto pred = [&description](const DescriptorSetData& descriptorSetData)
        {
            return description.size() == descriptorSetData.size();
        };

    Assert(!description.empty() && std::ranges::all_of(multiDescriptorSetData, pred));

    const vk::DescriptorSetLayout layout = VulkanContext::descriptorPool->CreateDescriptorSetLayout(description);

    const std::vector<vk::DescriptorSet> values
            = VulkanContext::descriptorPool->AllocateDescriptorSets(Repeat(layout, multiDescriptorSetData.size()));

    for (size_t i = 0; i < multiDescriptorSetData.size(); ++i)
    {
        VulkanContext::descriptorPool->UpdateDescriptorSet(values[i], multiDescriptorSetData[i], 0);
    }

    return MultiDescriptorSet{ layout, values };
}

void DescriptorHelpers::DestroyDescriptorSet(const DescriptorSet& descriptorSet)
{
    VulkanContext::descriptorPool->FreeDescriptorSets({ descriptorSet.value });
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(descriptorSet.layout);
}

void DescriptorHelpers::DestroyMultiDescriptorSet(const MultiDescriptorSet& multiDescriptorSet)
{
    if (!multiDescriptorSet.values.empty())
    {
        VulkanContext::descriptorPool->FreeDescriptorSets(multiDescriptorSet.values);
    }
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(multiDescriptorSet.layout);
}
