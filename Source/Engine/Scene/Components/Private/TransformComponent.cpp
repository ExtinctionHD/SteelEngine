#include "Engine/Scene/Components/TransformComponent.hpp"

void TransformComponent::SetLocalTransform(const Transform& transform)
{
    localTransform = transform;

    dirty = true;
}

void TransformComponent::SetLocalTranslation(const glm::vec3& translation)
{
    localTransform.SetTranslation(translation);
}

void TransformComponent::SetLocalRotation(const glm::quat& rotation)
{
    localTransform.SetRotation(rotation);
}

void TransformComponent::SetLocalScale(const glm::vec3& scale)
{
    localTransform.SetScale(scale);
}
