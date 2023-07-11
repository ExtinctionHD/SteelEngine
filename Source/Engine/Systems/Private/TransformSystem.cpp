#include "Engine/Systems/TransformSystem.hpp"

#include "Engine/Components/Components.hpp"
#include "Engine/Components/TransformComponent.hpp"
#include "Engine/Scene/Scene.hpp"

void TransformSystem::Process(Scene& scene, float)
{
    for (auto&& [entity, tc, hc] : scene.view<TransformComponent, HierarchyComponent>().each())
    {
        if (tc.dirty)
        {
            UpdateTransform(scene, entity, tc);

            UpdateChildrenTransform(scene, hc);
        }
    }
}

void TransformSystem::UpdateTransform(Scene& scene, entt::entity entity, TransformComponent& tc)
{
    tc.worldTransform = tc.localTransform;

    const HierarchyComponent& hc = scene.get<HierarchyComponent>(entity);

    if (hc.parent != entt::null)
    {
        TransformComponent& parentTc = scene.get<TransformComponent>(hc.parent);

        if (parentTc.dirty)
        {
            UpdateTransform(scene, hc.parent, parentTc);
        }

        tc.worldTransform *= parentTc.worldTransform;
    }

    tc.dirty = false;
}

void TransformSystem::UpdateChildrenTransform(Scene& scene, const HierarchyComponent& hc)
{
    for (const auto child : hc.children)
    {
        UpdateTransform(scene, child, scene.get<TransformComponent>(child));

        UpdateChildrenTransform(scene, scene.get<HierarchyComponent>(child));
    }
}
