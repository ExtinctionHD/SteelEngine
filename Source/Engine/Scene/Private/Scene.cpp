#include "Engine/Scene/Scene.hpp"

void Scene::AddNode(std::unique_ptr<Node> node)
{
    nodes.push_back(std::move(node));
}

void Scene::ForEachNode(NodeFunctor functor) const
{
    for (const auto &node : nodes)
    {
        ForEachNodeChild(GetRef(node), functor);

        functor(GetRef(node));
    }
}

void Scene::ForEachRenderObject(RenderObjectFunctor functor) const
{
    ForEachNode([&](const Node &node)
        {
            for (const auto &renderObject : node.renderObjects)
            {
                functor(GetRef(renderObject), node.transform);
            }
        });
}

void Scene::ForEachNodeChild(Node &node, NodeFunctor functor) const
{
    for (const auto &child : node.children)
    {
        ForEachNodeChild(GetRef(child), functor);

        functor(GetRef(child));
    }
}
