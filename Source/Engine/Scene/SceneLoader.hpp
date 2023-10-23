#pragma once

#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Scene/Components/AnimationComponent.hpp"

class Scene;

namespace tinygltf
{
    class Model;
    class Node;
    struct Animation;
}

struct AnimationParseInfo final
{
    struct AnimParseMapping
    {
        const tinygltf::Animation& gltfAnim;
        Animation anim;
    };
    std::set<int> animatedNodesIndices;
    std::vector<AnimParseMapping> animationsMapping;
};

class SceneLoader
{
public:
    SceneLoader(Scene& scene_, const Filepath& path);

    ~SceneLoader();

private:
    Scene& scene;

    Filepath sceneDirectory;

    std::unique_ptr<tinygltf::Model> model;

    void LoadModel(const Filepath& path) const;

    void AddTextureStorageComponent() const;

    void AddMaterialStorageComponent() const;

    void AddGeometryStorageComponent() const;

    AnimationParseInfo AddAnimationStorage();

    void AddEntities(AnimationParseInfo& animationParseInfo) const;

    void FinalizeAnimationsSetup(const AnimationParseInfo& animPI);

    void AddRenderComponent(entt::entity entity, const tinygltf::Node& node) const;

    void AddCameraComponent(entt::entity entity, const tinygltf::Node& node) const;

    void AddLightComponent(entt::entity entity, const tinygltf::Node& node) const;

    void AddEnvironmentComponent(entt::entity entity, const tinygltf::Node& node) const;

    void AddAnimationComponent(entt::entity entity, AnimationParseInfo& animationParseInfo, int gltfNodeIndex) const;
};
