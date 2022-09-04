#include "Engine/Scene/Components.hpp"

#include "Engine/Scene/Scene.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"

#include "Shaders/Common/Common.h"

void ComponentHelpers::AccumulateTransform(Scene& scene, entt::entity entity)
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

void ComponentHelpers::CollectLights(const Scene& scene, const ByteAccess& dst)
{
    const auto sceneLightsView = scene.view<TransformComponent, LightComponent>();
    
    const DataAccess<gpu::Light> lightData = DataAccess<gpu::Light>(dst);

    Assert(lightData.size >= sceneLightsView.size_hint());
    
    uint32_t index = 0;

    for (auto&& [entity, tc, lc] : sceneLightsView.each())
    {
        if (lc.type == LightComponent::Type::eDirectional)
        {
            const glm::vec3 direction = tc.worldTransform * glm::vec4(Vector3::kX, 0.0f);

            lightData[index].location = glm::vec4(-direction, 0.0f);
        }
        else if (lc.type == LightComponent::Type::ePoint)
        {
            const glm::vec3 position = glm::vec3(tc.worldTransform[3]);

            lightData[index].location = glm::vec4(position, 1.0f);
        }
        
        lightData[index].color = glm::vec4(lc.color, 0.0f);

        ++index;
    }
}
