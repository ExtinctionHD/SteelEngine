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
