#pragma once

#include <entt/entity/registry.hpp>

#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Filesystem/Filepath.hpp"

class Transform;

// TODO move to ContextEntity.hpp
enum class ContextEntityTag
{
    eCamera,
    eSunLight,
    eAtmosphere,
    eEnvironment,
};

template <ContextEntityTag EntityTag, class ComponentType>
class ContextEntity
{
public:
    using Component = ComponentType;

    ContextEntity() = default;

    ContextEntity(entt::entity entity_)
        : entity(entity_)
    {}

    operator entt::entity() const { return entity; }
    operator bool() const { return entity != entt::null; }

    bool operator==(const ContextEntity& other) const { return entity == other.entity; }
    bool operator!=(const ContextEntity& other) const { return entity != other.entity; }

private:
    entt::entity entity = entt::null;
};

using CameraEntity = ContextEntity<ContextEntityTag::eCamera, struct CameraComponent>;
using SunLightEntity = ContextEntity<ContextEntityTag::eSunLight, struct LightComponent>;
using AtmosphereEntity = ContextEntity<ContextEntityTag::eAtmosphere, struct AtmosphereComponent>;
using EnvironmentEntity = ContextEntity<ContextEntityTag::eEnvironment, struct EnvironmentComponent>;

class Scene : public entt::registry
{
public:
    Scene();
    Scene(const Filepath& path);

    ~Scene();

    void EnumerateHierarchy(const SceneEntityFunc& func) const;

    void EnumerateDescendants(entt::entity entity, const SceneEntityFunc& func) const;

    void EnumerateAncestors(entt::entity entity, const SceneEntityFunc& func) const;

    entt::entity FindRootParent(entt::entity entity) const;

    entt::entity FindEntity(const std::string& name) const;

    entt::entity CreateEntity(entt::entity parent, const Transform& transform);

    entt::entity CloneEntity(entt::entity entity, const Transform& transform);

    const Transform& GetEntityTransform(entt::entity entity) const;

    void RemoveEntity(entt::entity entity);

    void RemoveChildren(entt::entity entity);

    void EmplaceScenePrefab(Scene&& scene, entt::entity entity);

    void EmplaceSceneInstance(entt::entity scene, entt::entity entity);

    entt::entity CreateSceneInstance(entt::entity scene, const Transform& transform);

    std::unique_ptr<Scene> EraseScenePrefab(entt::entity scene);

    int32_t GetSunLightIndex() const;

    uint32_t GetLightCount() const;

    template <class ContextEntityType>
    typename ContextEntityType::Component& GetContextComponent()
    {
        const ContextEntityType entity = ctx().get<ContextEntityType>();

        return get<typename ContextEntityType::Component>(entity);
    }

    template <class ContextEntityType>
    const typename ContextEntityType::Component& GetContextComponent() const
    {
        const ContextEntityType entity = ctx().get<ContextEntityType>();

        return get<typename ContextEntityType::Component>(entity);
    }

private:
    entity_type create() { return entt::registry::create(); }
};
