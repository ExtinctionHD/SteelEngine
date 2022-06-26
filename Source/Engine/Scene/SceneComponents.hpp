#pragma once

struct Texture;
struct SampledTexture;
struct Material;
struct Primitive;

struct SceneTextureComponent
{
    std::vector<Texture> images;
    std::vector<vk::Sampler> samplers;
    std::vector<SampledTexture> textures;
};

struct SceneMaterialComponent
{
    std::vector<Material> materials;
};

struct SceneGeometryComponent
{
    std::vector<Primitive> primitives;
};

struct SceneRayTracingComponent
{
    std::vector<vk::Buffer> indexBuffers;
    std::vector<vk::Buffer> vertexBuffers;
    std::vector<vk::AccelerationStructureKHR> blases;
};

struct SceneRenderComponent
{
    vk::Buffer materialBuffer;
    vk::AccelerationStructureKHR tlas;
};