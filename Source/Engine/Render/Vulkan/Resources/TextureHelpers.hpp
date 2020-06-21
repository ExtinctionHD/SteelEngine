#pragma once

struct Texture
{
    vk::Image image;
    vk::ImageView view;
};

struct SamplerDescription
{
    vk::Filter magFilter;
    vk::Filter minFilter;
    vk::SamplerMipmapMode mipmapMode;
    vk::SamplerAddressMode addressMode;
    std::optional<float> maxAnisotropy;
    float minLod;
    float maxLod;
};
