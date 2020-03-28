#pragma once

#include "Engine/Scene/Node.hpp"

using NodeFunction = std::function<void(Observer<Node>)>;

class Scene
{
public:
    void AddNode(std::unique_ptr<Node> node);

    void ForEachNode(NodeFunction func);

private:
    std::vector<std::unique_ptr<Node>> nodes;

    void ForEachNodeChild(Observer<Node> node, NodeFunction function) const;
};
