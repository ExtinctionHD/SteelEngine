#include "Engine/Scene/Node.hpp"

Node::Node(const Scene &scene_)
    : scene(scene_)
{}

Node::~Node()
{
    for (const auto &child : children)
    {
        delete child;
    }
}
