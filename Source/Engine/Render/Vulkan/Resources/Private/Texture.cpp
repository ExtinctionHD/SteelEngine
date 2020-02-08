#include "Engine/Render/Vulkan/Resources/Texture.hpp"

bool SamplerDescription::operator==(const SamplerDescription &other) const
{
    return magFilter == other.magFilter && minFilter == other.minFilter
            && mipmapMode == other.mipmapMode && addressMode == other.addressMode
            && maxAnisotropy == other.maxAnisotropy && minLod == other.minLod
            && maxLod == other.maxLod;
}
