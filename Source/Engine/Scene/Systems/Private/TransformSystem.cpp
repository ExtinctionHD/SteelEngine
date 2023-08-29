#include "Engine/Scene/Systems/TransformSystem.hpp"

#include "Engine/Engine.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Scene.hpp"

void TransformSystem::Process(Scene& scene, float)
{
    const auto& sceneView = scene.view<TransformComponent, HierarchyComponent>();

    for (auto&& [entity, tc, hc] : sceneView.each())
    {
        if (tc.updated)
        {
            tc.updated = false;
        }
    }

    bool transformUpdated = false;

    for (auto&& [entity, tc, hc] : sceneView.each())
    {
        if (tc.modified)
        {
            UpdateTransform(scene, entity, tc);

            UpdateChildrenTransform(scene, hc);

            transformUpdated = true;
        }
    }

    if (transformUpdated)
    {
        Engine::TriggerEvent(EventType::eHierarchyUpdate);
    }
}

void TransformSystem::UpdateTransform(Scene& scene, entt::entity entity, TransformComponent& tc)
{
    tc.worldTransform = tc.localTransform;

    const HierarchyComponent& hc = scene.get<HierarchyComponent>(entity);

    if (hc.GetParent() != entt::null)
    {
        TransformComponent& parentTc = scene.get<TransformComponent>(hc.GetParent());

        if (parentTc.modified)
        {
            UpdateTransform(scene, hc.GetParent(), parentTc);
        }

        tc.worldTransform *= parentTc.worldTransform;
    }

    tc.modified = false;
    tc.updated = true;
}

void TransformSystem::UpdateChildrenTransform(Scene& scene, const HierarchyComponent& hc)
{
    for (const auto child : hc.GetChildren())
    {
        UpdateTransform(scene, child, scene.get<TransformComponent>(child));

        UpdateChildrenTransform(scene, scene.get<HierarchyComponent>(child));
    }
}
