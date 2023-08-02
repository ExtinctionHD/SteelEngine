#pragma once

#include <entt/entity/registry.hpp>

#include "Engine/Filesystem/Filepath.hpp"

class Scene : public entt::registry
{
public:
    Scene(const Filepath& path);

    ~Scene();

    void AddScene(Scene&& scene, entt::entity spawn);

    // TODO implement Remove

    std::unique_ptr<Scene> testChild;

private:
};
