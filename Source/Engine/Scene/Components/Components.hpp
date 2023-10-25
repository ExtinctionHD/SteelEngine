#pragma once

#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Primitive.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/Transform.hpp"

struct Texture;
struct TextureSampler;

class HierarchyComponent
{
public:
    HierarchyComponent(Scene& scene_, entt::entity self_, entt::entity parent_);

    entt::entity GetParent() const
    {
        return parent;
    }

    const std::vector<entt::entity>& GetChildren() const
    {
        return children;
    }

    void SetParent(entt::entity parent_);

private:
    Scene& scene;

    const entt::entity self;

    entt::entity parent = entt::null;

    std::vector<entt::entity> children;
};

class TransformComponent
{
public:
    TransformComponent(Scene& scene_, entt::entity self_, const Transform& localTransform_);

    const Transform& GetLocalTransform() const
    {
        return localTransform;
    }

    const Transform& GetWorldTransform() const;

    void SetLocalTransform(const Transform& transform);

    void SetLocalTranslation(const glm::vec3& translation);

    void SetLocalRotation(const glm::quat& rotation);

    void SetLocalScale(const glm::vec3& scale);

private:
    Scene& scene;

    const entt::entity self;

    Transform localTransform;

    mutable Transform worldTransform;

    mutable bool modified = true;
};

struct ScenePrefabComponent
{
    StorageRange storageRange;
    std::unique_ptr<Scene> hierarchy;
    std::vector<entt::entity> instances;
};

struct SceneInstanceComponent
{
    entt::entity prefab = entt::null;
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
    LightType type = LightType::eDirectional;
    glm::vec3 color;
};

struct TextureStorageComponent
{
    std::vector<Texture> textures;
    std::vector<vk::Sampler> samplers;
    std::vector<TextureSampler> textureSamplers;
    bool updated = false;
};

struct MaterialStorageComponent
{
    std::vector<Material> materials;
    bool updated = false;
};

struct GeometryStorageComponent
{
    std::vector<Primitive> primitives;
    bool updated = false;
};

struct RenderContextComponent
{
    vk::Buffer lightBuffer;
    vk::Buffer materialBuffer;
    std::vector<vk::Buffer> frameBuffers;
};

struct RayTracingContextComponent
{
    vk::AccelerationStructureKHR tlas;
    uint32_t tlasInstanceCount = 0;
    bool updated = false;
};
