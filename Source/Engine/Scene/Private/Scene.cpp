#include "Engine/Scene/Scene.hpp"

const VertexFormat Scene::kVertexFormat{
    vk::Format::eR32G32B32Sfloat,   // position
    vk::Format::eR32G32B32Sfloat,   // normal
    vk::Format::eR32G32B32Sfloat,   // tangent
    vk::Format::eR32G32Sfloat,      // texCoord
};

const vk::IndexType Scene::kIndexType = vk::IndexType::eUint32;

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
