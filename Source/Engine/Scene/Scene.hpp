#pragma once

#include <entt/entity/registry.hpp>

#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Filesystem/Filepath.hpp"

class Transform;

class Scene : public entt::registry
{
public:
    Scene();
    Scene(const Filepath& path);

    ~Scene();

    void EnumerateDescendants(entt::entity entity, const SceneEntityFunc& func) const;

    void EnumerateAncestors(entt::entity entity, const SceneEntityFunc& func) const;

    void EnumerateRenderView(const SceneRenderFunc& func) const; // TODO start using

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

private:
};
