#pragma once

#include <entt/entity/registry.hpp>

#include "Engine/Filesystem/Filepath.hpp"

#include "Utils/AABBox.hpp"

struct CameraComponent;
struct EnvironmentComponent;

class Scene : public entt::registry
{
public:
    Scene(const Filepath& path);

    ~Scene();

    void AddScene(Scene&& scene, entt::entity spawn);
};

namespace SceneHelpers
{
    AABBox ComputeSceneBBox(const Scene& scene);
}
