#pragma once

#include "Engine/Scene/Node.hpp"

using NodeFunction = std::function<void(Node &)>;

class Scene
{
public:
    void AddNode(std::unique_ptr<Node> node);

    void ForEachNode(NodeFunction func);

private:
    std::vector<std::unique_ptr<Node>> nodes;

    void ForEachNodeChild(Node &node, NodeFunction function) const;
};
