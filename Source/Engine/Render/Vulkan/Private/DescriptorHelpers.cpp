#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

bool DescriptorDescription::operator==(const DescriptorDescription &other) const
{
    return type == other.type && stageFlags == other.stageFlags && bindingFlags == other.bindingFlags;
}

ImageInfo DescriptorHelpers::GetInfo(vk::Sampler sampler, vk::ImageView view)
{
    return { vk::DescriptorImageInfo(sampler, view, vk::ImageLayout::eShaderReadOnlyOptimal) };
}

ImageInfo DescriptorHelpers::GetInfo(vk::ImageView view)
{
    return { vk::DescriptorImageInfo(nullptr, view, vk::ImageLayout::eGeneral) };
}

BufferInfo DescriptorHelpers::GetInfo(vk::Buffer buffer)
{
    return { vk::DescriptorBufferInfo(buffer, 0, VK_WHOLE_SIZE) };
}

AccelerationStructureInfo DescriptorHelpers::GetInfo(const vk::AccelerationStructureNV &accelerationStructure)
{
    return vk::WriteDescriptorSetAccelerationStructureNV(1, &accelerationStructure);
}
