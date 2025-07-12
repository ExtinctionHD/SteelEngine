#include "Engine/Scene/Components/Components.hpp"

#include "Engine/Scene/Scene.hpp"

HierarchyComponent::HierarchyComponent(Scene& scene_, entt::entity self_, entt::entity parent_)
    : scene(scene_)
    , self(self_)
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
    : scene(scene_)
    , self(self_)
    , localTransform(localTransform_)
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

    Modify();
}

void TransformComponent::SetLocalTranslation(const glm::vec3& translation)
{
    localTransform.SetTranslation(translation);

    Modify();
}

void TransformComponent::SetLocalDirection(const glm::vec3& direction)
{
    localTransform.SetDirection(direction);

    Modify();
}

void TransformComponent::SetLocalRotation(const glm::quat& rotation)
{
    localTransform.SetRotation(rotation);

    Modify();
}

void TransformComponent::SetLocalScale(const glm::vec3& scale)
{
    localTransform.SetScale(scale);

    Modify();
}

void TransformComponent::TranslateLocal(const glm::vec3& translation)
{
    localTransform.Translate(translation);

    Modify();
}

void TransformComponent::RotateLocal(const glm::quat& rotation)
{
    localTransform.Rotate(rotation);

    Modify();
}

void TransformComponent::ScaleLocal(const glm::vec3& scale)
{
    localTransform.Scale(scale);

    Modify();
}

void TransformComponent::Modify() const
{
    modified = true;

    scene.EnumerateDescendants(self, [&](entt::entity child)
        {
            scene.get<TransformComponent>(child).modified = true;
        });
}

glm::vec4 LightComponent::GetLocation(const Transform& transform) const
{
    return type == LightType::ePoint
            ? glm::vec4(transform.GetTranslation(), 1.0f)
            : glm::vec4(-transform.GetDirection(), 0.0f);
}

gpu::Light LightComponent::GetGpuLight(const Transform& transform) const
{
    return gpu::Light{ GetLocation(transform), radiance, {} };
}
