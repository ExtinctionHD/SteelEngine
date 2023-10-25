#include "Engine/Scene/Components/Components.hpp"

#include "Engine/Scene/Scene.hpp"

HierarchyComponent::HierarchyComponent(Scene& scene_, entt::entity self_, entt::entity parent_)
    : scene(scene_), self(self_)
{
    Assert(self != entt::null);

    SetParent(parent_);
}

void HierarchyComponent::SetParent(entt::entity parent_)
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

TransformComponent::TransformComponent(Scene& scene_, entt::entity self_, const Transform& localTransform_)
    : scene(scene_), self(self_), localTransform(localTransform_)
{
    Assert(self != entt::null);
}

const Transform& TransformComponent::GetWorldTransform() const
{
    if (modified)
    {
        worldTransform = localTransform;

        const entt::entity parent = scene.get<HierarchyComponent>(self).GetParent();

        if (parent != entt::null)
        {
            worldTransform *= scene.get<TransformComponent>(parent).GetWorldTransform();
        }

        modified = false;
    }

    return worldTransform;
}

void TransformComponent::SetLocalTransform(const Transform& transform)
{
    localTransform = transform;

    modified = true;

    scene.EnumerateDescendants(
        self, [&](entt::entity child)
        { scene.get<TransformComponent>(child).modified = true; });
}

void TransformComponent::SetLocalTranslation(const glm::vec3& translation)
{
    localTransform.SetTranslation(translation);

    modified = true;

    scene.EnumerateDescendants(
        self, [&](entt::entity child)
        { scene.get<TransformComponent>(child).modified = true; });
}

void TransformComponent::SetLocalRotation(const glm::quat& rotation)
{
    localTransform.SetRotation(rotation);

    modified = true;

    scene.EnumerateDescendants(
        self, [&](entt::entity child)
        { scene.get<TransformComponent>(child).modified = true; });
}

void TransformComponent::SetLocalScale(const glm::vec3& scale)
{
    localTransform.SetScale(scale);

    modified = true;

    scene.EnumerateDescendants(
        self, [&](entt::entity child)
        { scene.get<TransformComponent>(child).modified = true; });
}
