#pragma once

#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Primitive.hpp"

struct Texture;
struct TextureSampler;

struct TextureStorageComponent
{
    std::vector<Texture> textures;
    std::vector<vk::Sampler> samplers;
    std::vector<TextureSampler> textureSamplers;
};

struct MaterialStorageComponent
{
    std::vector<Material> materials;
};

struct GeometryStorageComponent
{
    std::vector<Primitive> primitives;
};
