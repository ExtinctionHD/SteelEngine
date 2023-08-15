#pragma once

#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Primitive.hpp"
#include "Engine/Scene/Transform.hpp"
#include "Engine/Scene/Scene.hpp"

struct Texture;
struct TextureSampler;

class HierarchyComponent
{
public:
    HierarchyComponent(Scene& scene, entt::entity self_, entt::entity parent_);

    entt::entity GetParent() const { return parent; }

    const std::vector<entt::entity>& GetChildren() const { return children; }

    void SetParent(Scene& scene, entt::entity parent_);

private:
    entt::entity self = entt::null;
    entt::entity parent = entt::null;

    std::vector<entt::entity> children;
};

class TransformComponent
{
public:
    TransformComponent() = default;
    TransformComponent(const Transform& localTransform_);

    const Transform& GetLocalTransform() const { return localTransform; }

    const Transform& GetWorldTransform() const { return worldTransform; }

    bool HasBeenModified() const { return modified; }

    bool HasBeenUpdated() const { return updated; }

    void SetLocalTransform(const Transform& transform);

    void SetLocalTranslation(const glm::vec3& translation);

    void SetLocalRotation(const glm::quat& rotation);

    void SetLocalScale(const glm::vec3& scale);

private:
    Transform localTransform;
    Transform worldTransform;

    bool modified = true;
    bool updated = true;

    friend class TransformSystem;
};

struct ScenePrefabComponent
{
    StorageRange storageRange;
    std::unique_ptr<Scene> hierarchy;
    std::vector<entt::entity> instances;
};

struct SceneInstanceComponent
{
    entt::entity parent = entt::null;
};

struct NameComponent
{
    std::string name;
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
