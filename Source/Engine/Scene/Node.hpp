#pragma once

#include "Engine/Render/RenderObject.hpp"

class Scene;

class Node
{
public:
    const Scene &scene;

    std::string name;
    glm::mat4 transform = glm::mat4(1.0f);
    std::vector<std::unique_ptr<RenderObject>> renderObjects;

    Node *parent = nullptr;
    std::vector<std::unique_ptr<Node>> children;

private:
    Node(const Scene &scene_);

    friend class SceneLoader;
};
