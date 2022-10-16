#pragma once

#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Primitive.hpp"

struct Texture;
struct SampledTexture;

struct TextureStorageComponent
{
    std::vector<Texture> images;
    std::vector<vk::Sampler> samplers;
    std::vector<SampledTexture> textures;
};

struct MaterialStorageComponent
{
    std::vector<Material> materials;
};

struct GeometryStorageComponent
{
    std::vector<Primitive> primitives;
};

struct RayTracingStorageComponent
{
    std::vector<vk::Buffer> indexBuffers;
    std::vector<vk::Buffer> vertexBuffers;
    std::vector<vk::AccelerationStructureKHR> blases;
};

struct RenderStorageComponent
{
    vk::Buffer lightBuffer;
    vk::Buffer materialBuffer;
    vk::AccelerationStructureKHR tlas;
};
