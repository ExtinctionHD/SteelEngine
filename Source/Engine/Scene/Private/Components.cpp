#include "Engine/Scene/Components.hpp"

#include "Engine/Scene/Scene.hpp"

#include "Utils/Helpers.hpp"

void ComponentHelpers::AccumulateTransform(Scene& scene, entt::entity entity)
{
    TransformComponent& tc = scene.get<TransformComponent>(entity);

    const HierarchyComponent& hc = scene.get<HierarchyComponent>(entity);

    tc.worldTransform = tc.localTransform;

    entt::entity parent = hc.parent;

    while (parent != entt::null)
    {
        const TransformComponent& parentTc = scene.get<TransformComponent>(parent);
        const HierarchyComponent& parentHc = scene.get<HierarchyComponent>(parent);

        tc.worldTransform *= parentTc.localTransform;

        parent = parentHc.parent;
    }
}

std::vector<gpu::Light> ComponentHelpers::CollectLights(const Scene& scene)
{
    const auto sceneLightsView = scene.view<TransformComponent, LightComponent>();

    std::vector<gpu::Light> lights;
    lights.reserve(sceneLightsView.size_hint());

    for (auto&& [entity, tc, lc] : sceneLightsView.each())
    {
        gpu::Light light{};

        if (lc.type == LightComponent::Type::eDirectional)
        {
            const glm::vec3 direction = tc.worldTransform.GetAxis(Axis::eX);

            light.location = glm::vec4(-direction, 0.0f);
        }
        else if (lc.type == LightComponent::Type::ePoint)
        {
            const glm::vec3 position = tc.worldTransform.GetTranslation();

            light.location = glm::vec4(position, 1.0f);
        }

        light.color = glm::vec4(lc.color, 0.0f);

        lights.push_back(light);
    }

    return lights;
}
