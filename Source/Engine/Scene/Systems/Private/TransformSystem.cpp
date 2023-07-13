#include "Engine/Scene/Systems/TransformSystem.hpp"

#include "Engine/Config.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Components/TransformComponent.hpp"
#include "Engine/Render/SceneRenderer.hpp"
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

    if constexpr (Config::kRayTracingEnabled)
    {
    }
}

void TransformSystem::UpdateChildrenTransform(Scene& scene, const HierarchyComponent& hc)
{
    for (const auto child : hc.children)
    {
        UpdateTransform(scene, child, scene.get<TransformComponent>(child));

        UpdateChildrenTransform(scene, scene.get<HierarchyComponent>(child));
    }
}
