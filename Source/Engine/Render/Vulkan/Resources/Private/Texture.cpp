#include "Engine/Render/Vulkan/Resources/Texture.hpp"

bool SamplerDescription::operator==(const SamplerDescription &other) const
{
    return magFilter == other.magFilter && minFilter == other.minFilter
            && mipmapMode == other.mipmapMode && addressMode == other.addressMode
            && maxAnisotropy == other.maxAnisotropy && minLod == other.minLod
            && maxLod == other.maxLod;
}

vk::DescriptorImageInfo TextureHelpers::GetInfo(const Texture &texture)
{
    return vk::DescriptorImageInfo(texture.sampler, texture.view, vk::ImageLayout::eShaderReadOnlyOptimal);
}
