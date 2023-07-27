#pragma once

class Scene;
class Filepath;

namespace tinygltf
{
    class Model;
    class Node;
}

class SceneLoader
{
public:
    SceneLoader(Scene& scene_, const Filepath& path);

    ~SceneLoader();

private:
    Scene& scene;

    std::unique_ptr<tinygltf::Model> model;

    void LoadModel(const Filepath& path) const;

    void AddTextureStorageComponent() const;

    void AddMaterialStorageComponent() const;

    void AddGeometryStorageComponent() const;

    void AddEntities() const;

    void AddHierarchyComponent(entt::entity entity, entt::entity parent) const;

    void AddTransformComponent(entt::entity entity, const tinygltf::Node& node) const;

    void AddNameComponent(entt::entity entity, const tinygltf::Node& node) const;

    void AddRenderComponent(entt::entity entity, const tinygltf::Node& node) const;

    void AddCameraComponent(entt::entity entity, const tinygltf::Node& node) const;

    void AddLightComponent(entt::entity entity, const tinygltf::Node& node) const;

    void AddEnvironmentComponent(entt::entity entity, const tinygltf::Node& node) const;

    void AddScene(entt::entity entity, const tinygltf::Node& node) const;
};
