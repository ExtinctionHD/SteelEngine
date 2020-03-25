#pragma once

#include "Engine/Scene/Node.hpp"

using NodeFunction = std::function<void(NodeHandle)>;

class Scene
{
public:
    Scene() = default;
    ~Scene();

    void AddNode(NodeHandle node);

    void ForEachNode(NodeFunction func);

private:
    std::vector<NodeHandle> nodes;

    void ForEachNodeChild(NodeHandle node, NodeFunction function);
};
