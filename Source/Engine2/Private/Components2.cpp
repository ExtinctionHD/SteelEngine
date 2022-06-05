#include "Engine2/Components2.hpp"

glm::mat4 TransformComponent::AccumulateTransform(const TransformComponent& tc)
{
    glm::mat4 transform = tc.localTransform;

    TransformComponent* parent = tc.parent;

    while(parent != nullptr)
    {
        transform = parent->localTransform * transform;
        parent = parent->parent;
    }

    return transform;
}
