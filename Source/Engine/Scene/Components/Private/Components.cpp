#include "Engine/Scene/Components/Components.hpp"

#include "Engine/Scene/Scene.hpp"

HierarchyComponent::HierarchyComponent(Scene& scene, entt::entity self_, entt::entity parent_)
    : self(self_)
{
    SetParent(scene, parent_);
}

void HierarchyComponent::SetParent(Scene& scene, entt::entity parent_)
{
    if (parent == parent_)
    {
        return;
    }

    if (parent != entt::null)
    {
        auto& parentHc = scene.get<HierarchyComponent>(parent);

        std::erase(parentHc.children, self);
    }

    parent = parent_;

    if (parent != entt::null)
    {
        auto& parentHc = scene.get<HierarchyComponent>(parent);

        parentHc.children.push_back(self);
    }
}

TransformComponent::TransformComponent(const Transform& localTransform_)
    : localTransform(localTransform_)
{}

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
