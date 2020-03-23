#pragma once

#include "Engine/Render/RenderObject.hpp"

#include "Utils/Helpers.hpp"

class Node;
class Scene;

using NodeHandle = const Node *;

class Node
{
public:
    const Scene &scene;

    std::string name;
    glm::mat4 transform = Matrix4::kIdentity;
    std::vector<RenderObject> renderObjects;

    NodeHandle parent = nullptr;
    std::vector<NodeHandle> children;

private:
    Node(const Scene &scene_);
    ~Node();

    friend class Scene;
    friend class SceneLoader;
};
