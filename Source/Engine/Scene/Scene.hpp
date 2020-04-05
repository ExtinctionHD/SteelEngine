#pragma once

#include "Engine/Scene/Node.hpp"

using NodeFunctor = std::function<void(Node &)>;

class Scene
{
public:
    void AddNode(std::unique_ptr<Node> node);

    void ForEachNode(NodeFunctor functor) const;

private:
    std::vector<std::unique_ptr<Node>> nodes;

    void ForEachNodeChild(Node &node, NodeFunctor functor) const;
};
