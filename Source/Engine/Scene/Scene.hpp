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

    void EnumerateHierarchy(entt::entity entity, const SceneEntityFunc& func) const;

    void EnumerateChildren(entt::entity parent, const SceneEntityFunc& func) const;

    void EnumerateRenderView(const SceneRenderFunc& func) const; // TODO start using

    entt::entity FindEntity(const std::string& name) const;

    entt::entity AddEntity(entt::entity parent, const Transform& transform);

    entt::entity CloneEntity(entt::entity entity, const Transform& transform);

    void RemoveEntity(entt::entity entity);

    void RemoveChildren(entt::entity entity);

    void InsertScene(Scene&& scene, entt::entity entity);

    void InstantiateScene(entt::entity scene, entt::entity entity);

    std::unique_ptr<Scene> EraseScene(entt::entity scene);

private:
};
