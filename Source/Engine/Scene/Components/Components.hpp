#pragma once

#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Primitive.hpp"
#include "Engine/Scene/Transform.hpp"
#include "Engine/Scene/Scene.hpp"

#include "Utils/Color.hpp"

struct Texture;

// TODO move to separate file
class HierarchyComponent
{
public:
    HierarchyComponent(Scene& scene_, entt::entity self_, entt::entity parent_);

    entt::entity GetParent() const { return parent; }

    const std::vector<entt::entity>& GetChildren() const { return children; }

    bool HasNoChildren() const { return children.empty(); }

    bool HasSingleChild() const { return children.size() == 1; }

    void SetParent(entt::entity parent_);

private:
    Scene& scene;

    const entt::entity self;

    entt::entity parent = entt::null;

    std::vector<entt::entity> children;
};

// TODO move to separate file
class TransformComponent
{
public:
    TransformComponent(Scene& scene_, entt::entity self_, const Transform& localTransform_);

    const Transform& GetLocalTransform() const { return localTransform; }

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
    ePoint,
    eDirectional
};

struct LightComponent
{
    LightType type = LightType::ePoint;
    LinearColor color;
};

struct AtmosphereComponent
{
    float planetRadius = 6360'000.0f;
    float atmosphereRadius = 6460'000.0f;
    glm::vec3 rayleightScattering = glm::vec3(5.802f, 13.558f, 33.1f);
    float rayleightDensityHeight = 8'000.0f;
    float mieScattering = 3.996f;
    float mieAbsorption = 4.4f;
    float mieDensityHeight = 1'200.0f;
    float mieScatteringAsymmetry = 0.8f;
    glm::vec3 ozoneAbsorption = glm::vec3(0.65f, 1.881f, 0.085f);
    float ozoneCenterHeight = 25'000.0f;
    float ozoneThickness = 30'000.0f;
};

// TODO move storage components to separate files
struct TextureStorageComponent
{
    std::vector<Texture> textures;
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
