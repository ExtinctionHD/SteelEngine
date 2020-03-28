#include "Engine/Scene/Scene.hpp"

void Scene::AddNode(std::unique_ptr<Node> node)
{
    nodes.push_back(std::move(node));
}

void Scene::ForEachNode(NodeFunction func)
{
    for (const auto &node : nodes)
    {
        ForEachNodeChild(GetObserver(node), func);

        func(GetObserver(node));
    }
}

void Scene::ForEachNodeChild(Observer<Node> node, NodeFunction function) const
{
    for (const auto &child : node->children)
    {
        ForEachNodeChild(GetObserver(child), function);

        function(GetObserver(child));
    }
}
