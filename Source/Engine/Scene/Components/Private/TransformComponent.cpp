#include "Engine/Scene/Components/TransformComponent.hpp"

void TransformComponent::SetLocalTransform(const Transform& transform)
{
    localTransform = transform;

    modified = true;
}

void TransformComponent::SetLocalTranslation(const glm::vec3& translation)
{
    localTransform.SetTranslation(translation);

    modified = true;
}

void TransformComponent::SetLocalRotation(const glm::quat& rotation)
{
    localTransform.SetRotation(rotation);

    modified = true;
}

void TransformComponent::SetLocalScale(const glm::vec3& scale)
{
    localTransform.SetScale(scale);

    modified = true;
}
