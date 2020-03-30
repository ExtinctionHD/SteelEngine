#include "Engine/Scene/Scene.hpp"

void Scene::AddNode(std::unique_ptr<Node> node)
{
    nodes.push_back(std::move(node));
}

void Scene::ForEachNode(NodeFunction function)
{
    for (const auto &node : nodes)
    {
        ForEachNodeChild(GetRef(node), function);

        function(GetRef(node));
    }
}

void Scene::ForEachNodeChild(Node &node, NodeFunction function) const
{
    for (const auto &child : node.children)
    {
        ForEachNodeChild(GetRef(child), function);

        function(GetRef(child));
    }
}
