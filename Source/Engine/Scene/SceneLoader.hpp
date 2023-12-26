#pragma once

#include "Engine/Filesystem/Filepath.hpp"

class Scene;

namespace tinygltf
{
    class Model;
    class Node;
    struct Animation;
}

class SceneLoader
{
public:
    SceneLoader(Scene& scene_, const Filepath& path);

    ~SceneLoader();

private:
    using EntityMap = std::map<int32_t, entt::entity>;

    Scene& scene;
    Filepath sceneDirectory;

    std::unique_ptr<tinygltf::Model> model;

    void LoadModel(const Filepath& path) const;

    void AddTextureStorageComponent() const;

    void AddMaterialStorageComponent() const;

    void AddGeometryStorageComponent() const;

    EntityMap AddEntities() const;
    
    void AddRenderComponent(entt::entity entity, const tinygltf::Node& node) const;

    void AddCameraComponent(entt::entity entity, const tinygltf::Node& node) const;

    void AddLightComponent(entt::entity entity, const tinygltf::Node& node) const;

    void AddEnvironmentComponent(entt::entity entity, const tinygltf::Node& node) const;

    void AddAnimationComponent(const EntityMap& entityMap) const;
};
