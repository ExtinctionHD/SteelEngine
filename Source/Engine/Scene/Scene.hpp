#pragma once

#include "Engine/Scene/Node.hpp"

using NodeFunctor = std::function<void(const Node &)>;
using RenderObjectFunctor = std::function<void(const RenderObject &, const glm::mat4 &)>;

class Scene
{
public:
    void AddNode(std::unique_ptr<Node> node);

    void ForEachNode(NodeFunctor functor) const;

    void ForEachRenderObject(RenderObjectFunctor functor) const;

private:
    Scene() = default;

    std::vector<std::unique_ptr<Node>> nodes;

    void ForEachNodeChild(Node &node, NodeFunctor functor) const;

    friend class SceneLoader;
};
