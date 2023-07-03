#include "Engine/Render/Vulkan/Resources/DescriptorHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace Details
{
    vk::DescriptorImageInfo RetrieveTextureSampler(const DescriptorSource& source)
    {
        const auto& [view, sampler] = std::get<TextureSampler>(source);

        return vk::DescriptorImageInfo(sampler, view, vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    ImageInfo RetrieveSamplers(const DescriptorSources& sources)
    {
        Assert(std::get<const std::vector<vk::Sampler>*>(sources));

        const std::vector<vk::Sampler>& samplers = *std::get<const std::vector<vk::Sampler>*>(sources);

        ImageInfo imageInfo;
        imageInfo.reserve(samplers.size());

        for (const auto& sampler : samplers)
        {
            imageInfo.emplace_back(sampler);
        }

        return imageInfo;
    }

    ImageInfo RetrieveTextureSamplers(const DescriptorSources& sources)
    {
        Assert(std::get<const std::vector<TextureSampler>*>(sources));

        const std::vector<TextureSampler>& textureSamplers = *std::get<const std::vector<TextureSampler>*>(sources);

        ImageInfo imageInfo;
        imageInfo.reserve(textureSamplers.size());

        for (const auto& [view, sampler] : textureSamplers)
        {
            imageInfo.emplace_back(sampler, view, vk::ImageLayout::eShaderReadOnlyOptimal);
        }

        return imageInfo;
    }

    ImageInfo RetrieveImageViews(const DescriptorSources& sources, vk::ImageLayout layout)
    {
        Assert(std::get<const std::vector<vk::ImageView>*>(sources));

        const std::vector<vk::ImageView>& views = *std::get<const std::vector<vk::ImageView>*>(sources);

        ImageInfo imageInfo;
        imageInfo.reserve(views.size());

        for (const auto& view : views)
        {
            imageInfo.emplace_back(nullptr, view, layout);
        }

        return imageInfo;
    }

    BufferInfo RetrieveBuffers(const DescriptorSources& sources)
    {
        Assert(std::get<const std::vector<vk::Buffer>*>(sources));

        const std::vector<vk::Buffer>& buffers = *std::get<const std::vector<vk::Buffer>*>(sources);

        BufferInfo bufferInfo;
        bufferInfo.reserve(buffers.size());

        for (const auto& buffer : buffers)
        {
            bufferInfo.emplace_back(buffer, 0, VK_WHOLE_SIZE);
        }

        return bufferInfo;
    }

    const BufferViews& RetrieveBufferViews(const DescriptorSources& sources)
    {
        Assert(std::get<const std::vector<vk::BufferView>*>(sources));
        return *std::get<const std::vector<vk::BufferView>*>(sources);
    }

    AccelerationStructureInfo RetrieveAccelerationStructures(const DescriptorSources& sources)
    {
        Assert(std::get<const std::vector<vk::AccelerationStructureKHR>*>(sources));

        const std::vector<vk::AccelerationStructureKHR>& accelerationStructures
                = *std::get<const std::vector<vk::AccelerationStructureKHR>*>(sources);

        const uint32_t count = static_cast<uint32_t>(accelerationStructures.size());

        return AccelerationStructureInfo(count, accelerationStructures.data());
    }
}

bool DescriptorKey::operator==(const DescriptorKey& other) const
{
    return set == other.set && binding == other.binding;
}

bool DescriptorKey::operator<(const DescriptorKey& other) const
{
    if (set == other.set)
    {
        return binding < other.binding;
    }

    return set < other.set;
}

bool DescriptorDescription::operator==(const DescriptorDescription& other) const
{
    return key == other.key && count == other.count && type == other.type
            && stageFlags == other.stageFlags && bindingFlags == other.bindingFlags;
}

bool DescriptorDescription::operator<(const DescriptorDescription& other) const
{
    if (key == other.key)
    {
        if (count == other.count)
        {
            if (type == other.type)
            {
                if (stageFlags == other.stageFlags)
                {
                    return bindingFlags < other.bindingFlags;
                }

                return stageFlags < other.stageFlags;
            }

            return type < other.type;
        }

        return count < other.count;
    }

    return key < other.key;
}

DescriptorData DescriptorHelpers::GetData(vk::DescriptorType type, const DescriptorSource& source)
{
    DescriptorData descriptorData{};
    descriptorData.type = type;

    if (std::holds_alternative<std::monostate>(source))
    {
        descriptorData.descriptorInfo = std::monostate();
        return descriptorData;
    }

    switch (type)
    {
    case vk::DescriptorType::eSampler:
        descriptorData.descriptorInfo = ImageInfo{
            vk::DescriptorImageInfo(std::get<vk::Sampler>(source))
        };
        break;
    case vk::DescriptorType::eCombinedImageSampler:
        descriptorData.descriptorInfo = ImageInfo{
            Details::RetrieveTextureSampler(source)
        };
        break;
    case vk::DescriptorType::eSampledImage:
        descriptorData.descriptorInfo = ImageInfo{
            vk::DescriptorImageInfo(nullptr,
                    std::get<vk::ImageView>(source),
                    vk::ImageLayout::eShaderReadOnlyOptimal)
        };
        break;
    case vk::DescriptorType::eStorageImage:
        descriptorData.descriptorInfo = ImageInfo{
            vk::DescriptorImageInfo(nullptr,
                    std::get<vk::ImageView>(source),
                    vk::ImageLayout::eGeneral)
        };
        break;
    case vk::DescriptorType::eInputAttachment:
        descriptorData.descriptorInfo = ImageInfo{
            vk::DescriptorImageInfo(nullptr,
                    std::get<vk::ImageView>(source),
                    vk::ImageLayout::eShaderReadOnlyOptimal)
        };
        break;
    case vk::DescriptorType::eUniformBuffer:
    case vk::DescriptorType::eStorageBuffer:
    case vk::DescriptorType::eUniformBufferDynamic:
    case vk::DescriptorType::eStorageBufferDynamic:
        descriptorData.descriptorInfo = BufferInfo{
            vk::DescriptorBufferInfo(std::get<vk::Buffer>(source), 0, VK_WHOLE_SIZE)
        };
        break;
    case vk::DescriptorType::eUniformTexelBuffer:
    case vk::DescriptorType::eStorageTexelBuffer:
        descriptorData.descriptorInfo = BufferViews{ std::get<vk::BufferView>(source) };
        break;
    case vk::DescriptorType::eAccelerationStructureKHR:
        Assert(std::get<const vk::AccelerationStructureKHR*>(source));
        descriptorData.descriptorInfo = AccelerationStructureInfo(1,
                std::get<const vk::AccelerationStructureKHR*>(source));
        break;
    case vk::DescriptorType::eInlineUniformBlockEXT:
    default:
        Assert(false);
        break;
    }

    return descriptorData;
}

DescriptorData DescriptorHelpers::GetData(vk::Sampler sampler)
{
    return GetData(vk::DescriptorType::eSampler, sampler);
}

DescriptorData DescriptorHelpers::GetData(vk::ImageView view, vk::Sampler sampler)
{
    return GetData(TextureSampler{ view, sampler });
}

DescriptorData DescriptorHelpers::GetData(const TextureSampler& textureSampler)
{
    return GetData(vk::DescriptorType::eCombinedImageSampler, textureSampler);
}

DescriptorData DescriptorHelpers::GetData(vk::Buffer buffer)
{
    return GetData(vk::DescriptorType::eUniformBuffer, buffer);
}

DescriptorData DescriptorHelpers::GetData(const vk::AccelerationStructureKHR& accelerationStructure)
{
    return GetData(vk::DescriptorType::eAccelerationStructureKHR, &accelerationStructure);
}

DescriptorData DescriptorHelpers::GetStorageData(vk::ImageView view)
{
    return GetData(vk::DescriptorType::eStorageImage, view);
}

DescriptorData DescriptorHelpers::GetStorageData(vk::Buffer buffer)
{
    return GetData(vk::DescriptorType::eStorageBuffer, buffer);
}

DescriptorData DescriptorHelpers::GetData(vk::DescriptorType type, const DescriptorSources& sources)
{
    DescriptorData descriptorData{};
    descriptorData.type = type;

    if (std::holds_alternative<std::monostate>(sources))
    {
        descriptorData.descriptorInfo = std::monostate();
        return descriptorData;
    }

    switch (type)
    {
    case vk::DescriptorType::eSampler:
        descriptorData.descriptorInfo = Details::RetrieveSamplers(sources);
        break;
    case vk::DescriptorType::eCombinedImageSampler:
        descriptorData.descriptorInfo = Details::RetrieveTextureSamplers(sources);
        break;
    case vk::DescriptorType::eSampledImage:
        descriptorData.descriptorInfo = Details::RetrieveImageViews(sources, vk::ImageLayout::eShaderReadOnlyOptimal);
        break;
    case vk::DescriptorType::eStorageImage:
        descriptorData.descriptorInfo = Details::RetrieveImageViews(sources, vk::ImageLayout::eGeneral);
        break;
    case vk::DescriptorType::eInputAttachment:
        descriptorData.descriptorInfo = Details::RetrieveImageViews(sources, vk::ImageLayout::eShaderReadOnlyOptimal);
        break;
    case vk::DescriptorType::eUniformBuffer:
    case vk::DescriptorType::eStorageBuffer:
    case vk::DescriptorType::eUniformBufferDynamic:
    case vk::DescriptorType::eStorageBufferDynamic:
        descriptorData.descriptorInfo = Details::RetrieveBuffers(sources);
        break;
    case vk::DescriptorType::eUniformTexelBuffer:
    case vk::DescriptorType::eStorageTexelBuffer:
        descriptorData.descriptorInfo = Details::RetrieveBufferViews(sources);
        break;
    case vk::DescriptorType::eAccelerationStructureKHR:
        descriptorData.descriptorInfo = Details::RetrieveAccelerationStructures(sources);
        break;
    case vk::DescriptorType::eInlineUniformBlockEXT:
    default:
        Assert(false);
        break;
    }

    return descriptorData;
}

DescriptorData DescriptorHelpers::GetData(const std::vector<vk::ImageView>& views, vk::Sampler sampler)
{
    ImageInfo imageInfo;
    imageInfo.reserve(views.size());

    for (const auto& view : views)
    {
        imageInfo.emplace_back(sampler, view, vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    return DescriptorData{ vk::DescriptorType::eCombinedImageSampler, imageInfo };
}

DescriptorData DescriptorHelpers::GetData(const std::vector<TextureSampler>& textureSamplers)
{
    return GetData(vk::DescriptorType::eCombinedImageSampler, &textureSamplers);
}

DescriptorData DescriptorHelpers::GetData(const std::vector<vk::Buffer>& buffers)
{
    return GetData(vk::DescriptorType::eUniformBuffer, &buffers);
}

DescriptorData DescriptorHelpers::GetStorageData(const std::vector<vk::ImageView>& views)
{
    return GetData(vk::DescriptorType::eStorageImage, &views);
}

DescriptorData DescriptorHelpers::GetStorageData(const std::vector<vk::Buffer>& buffers)
{
    return GetData(vk::DescriptorType::eStorageBuffer, &buffers);
}

bool DescriptorHelpers::WriteDescriptorData(vk::WriteDescriptorSet& write, const DescriptorData& data)
{
    const auto& [descriptorType, descriptorInfo] = data;

    if (std::holds_alternative<std::monostate>(descriptorInfo))
    {
        return false;
    }

    switch (descriptorType)
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
            return false;
        }

        write.descriptorCount = static_cast<uint32_t>(imageInfo.size());
        write.pImageInfo = imageInfo.data();
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
            return false;
        }

        write.descriptorCount = static_cast<uint32_t>(bufferInfo.size());
        write.pBufferInfo = bufferInfo.data();
        break;
    }
    case vk::DescriptorType::eUniformTexelBuffer:
    case vk::DescriptorType::eStorageTexelBuffer:
    {
        const BufferViews& bufferViews = std::get<BufferViews>(descriptorInfo);

        if (bufferViews.empty())
        {
            return false;
        }

        write.descriptorCount = static_cast<uint32_t>(bufferViews.size());
        write.pTexelBufferView = bufferViews.data();
        break;
    }
    case vk::DescriptorType::eAccelerationStructureKHR:
    {
        const AccelerationStructureInfo& accelerationStructureInfo
                = std::get<AccelerationStructureInfo>(descriptorInfo);

        if (accelerationStructureInfo.accelerationStructureCount == 0)
        {
            return false;
        }

        write.descriptorCount = 1;
        write.pNext = &accelerationStructureInfo;
        break;
    }
    case vk::DescriptorType::eInlineUniformBlockEXT:
    default:
        Assert(false);
        break;
    }

    return true;
}
