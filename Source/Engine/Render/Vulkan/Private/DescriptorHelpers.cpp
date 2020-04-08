#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

#include "Engine/Render/Vulkan/Resources/Texture.hpp"

bool DescriptorDescription::operator==(const DescriptorDescription &other) const
{
    return type == other.type && stageFlags == other.stageFlags && bindingFlags == other.bindingFlags;
}

ImageInfo DescriptorHelpers::GetInfo(const Texture &texture)
{
    return { vk::DescriptorImageInfo(texture.sampler, texture.view, Texture::kLayout) };
}

BufferInfo DescriptorHelpers::GetInfo(vk::Buffer buffer)
{
    return { vk::DescriptorBufferInfo(buffer, 0, VK_WHOLE_SIZE) };
}

AccelerationStructureInfo DescriptorHelpers::GetInfo(const vk::AccelerationStructureNV &accelerationStructure)
{
    return vk::WriteDescriptorSetAccelerationStructureNV(1, &accelerationStructure);
}
