#include "Engine/Scene/Scene.hpp"

Scene::~Scene()
{
    for (const auto &node : nodes)
    {
        delete node;
    }
}

void Scene::AddNode(NodeHandle node)
{
    nodes.push_back(node);
}

void Scene::ForEachNode(NodeFunction func)
{
    for (const auto &node : nodes)
    {
        ForEachNodeChild(node, func);

        func(node);
    }
}

void Scene::ForEachNodeChild(NodeHandle node, NodeFunction function)
{
    for (const auto &child : node->children)
    {
        ForEachNodeChild(child, function);

        function(child);
    }
}
