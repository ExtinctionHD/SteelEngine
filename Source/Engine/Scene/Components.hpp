#pragma once

#include "Utils/DataHelpers.hpp"

class Scene;

struct HierarchyComponent
{
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
};

struct TransformComponent
{
    glm::mat4 localTransform;
    glm::mat4 worldTransform;
};

struct RenderObject
{
    uint32_t primitive = 0;
    uint32_t material = 0;
};

struct RenderComponent
{
    std::vector<RenderObject> renderObjects;
};

struct LightComponent
{
    enum class Type
    {
        eDirectional,
        ePoint
    };

    Type type = Type::eDirectional;
    glm::vec3 color;
};

namespace ComponentHelpers
{
    void AccumulateTransform(Scene& scene, entt::entity entity);

    void CollectLights(const Scene& scene, const ByteAccess& dst);
}
