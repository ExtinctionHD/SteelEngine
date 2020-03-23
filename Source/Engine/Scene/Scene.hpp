#pragma once

#include "Engine/Scene/Node.hpp"

class Scene
{
public:
    Scene() = default;
    ~Scene();

    void AddNode(NodeHandle node);

private:
    std::vector<NodeHandle> nodes;
};
