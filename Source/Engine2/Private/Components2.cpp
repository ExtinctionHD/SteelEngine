#include "Engine2/Components2.hpp"

#include "Engine2/Scene2.hpp"

void ComponentHelpers::AccumulateTransform(Scene2& scene, entt::entity entity)
{
    TransformComponent& tc = scene.get<TransformComponent>(entity);

    const HierarchyComponent& hc = scene.get<HierarchyComponent>(entity);

    glm::mat4 transform = tc.localTransform;
    entt::entity parent = hc.parent;

    while (parent != entt::null)
    {
        const TransformComponent& parentTc = scene.get<TransformComponent>(parent);
        const HierarchyComponent& parentHc = scene.get<HierarchyComponent>(parent);

        transform = parentTc.localTransform * transform;
        parent = parentHc.parent;
    }

    tc.worldTransform = transform;
}
