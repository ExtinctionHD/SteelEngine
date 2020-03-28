#pragma once

#include "Utils/Helpers.hpp"

struct Texture
{
    vk::Image image;
    vk::ImageView view;
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

    bool operator ==(const SamplerDescription &other) const;
};

namespace std
{
    template <>
    struct hash<SamplerDescription>
    {
        size_t operator()(const SamplerDescription &description) const noexcept
        {
            size_t result = 0;

            CombineHash(result, description.magFilter);
            CombineHash(result, description.minFilter);
            CombineHash(result, description.mipmapMode);
            CombineHash(result, description.addressMode);
            CombineHash(result, description.maxAnisotropy);
            CombineHash(result, description.minLod);
            CombineHash(result, description.maxLod);

            return result;
        }
    };
}
