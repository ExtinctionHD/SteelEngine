#pragma once

#include "Engine/Render/RenderObject.hpp"

#include "Utils/Helpers.hpp"

class Scene;

class Node
{
public:
    const Scene &scene;

    std::string name;
    glm::mat4 transform = glm::mat4(1.0f);
    std::vector<RenderObject> renderObjects;

    Observer<Node> parent = nullptr;
    std::vector<std::unique_ptr<Node>> children;

private:
    Node(const Scene &scene_);

    friend class Scene;
    friend class SceneLoader;
};
