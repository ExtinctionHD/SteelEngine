#pragma once

#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Filesystem.hpp"

#include "Utils/Helpers.hpp"

struct Texture
{
    ImageHandle image;
    vk::Sampler sampler;
};

struct SamplerDescription
{
    vk::Filter magFilter = vk::Filter::eLinear;
    vk::Filter minFilter = vk::Filter::eLinear;
    vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear;
    vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat;
    std::optional<float> maxAnisotropy = 16.0f;
    float minLod = 0.0f;
    float maxLod = 0.0f;

    bool operator ==(const SamplerDescription& other) const;
};

namespace std
{
    template <>
    struct hash<SamplerDescription>
    {
        size_t operator()(const SamplerDescription& description) const noexcept
        {
            size_t result = 0;

            HashCombine(result, description.magFilter);
            HashCombine(result, description.minFilter);
            HashCombine(result, description.mipmapMode);
            HashCombine(result, description.addressMode);
            HashCombine(result, description.maxAnisotropy);
            HashCombine(result, description.minLod);
            HashCombine(result, description.maxLod);

            return result;
        }
    };
}
