#pragma once

#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Primitive.hpp"

class Scene;
struct Texture;
struct TextureSampler;

struct NameComponent
{
    std::string name;
};

struct HierarchyComponent
{
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
};

struct RenderObject
{
    uint32_t primitive = 0;
    uint32_t material = 0;
};

struct RenderComponent
{
    std::vector<RenderObject> renderObjects;
};

enum class LightType
{
    eDirectional,
    ePoint
};

struct LightComponent
{
    LightType type;
    glm::vec3 color;
};

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

struct RenderSceneComponent
{
    vk::Buffer lightBuffer;
    vk::Buffer materialBuffer;
    std::vector<vk::Buffer> frameBuffers;

    uint32_t updateLightBuffer : 1;
    uint32_t updateMaterialBuffer : 1;
};

struct RayTracingSceneComponent
{
    vk::AccelerationStructureKHR tlas;
};
