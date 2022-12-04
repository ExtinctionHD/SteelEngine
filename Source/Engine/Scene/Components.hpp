#pragma once

#include "Utils/Transform.hpp"

#include "Shaders/Common/Common.h"

class Scene;

struct HierarchyComponent
{
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
};

struct TransformComponent
{
    Transform localTransform;
    Transform worldTransform;
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

    std::vector<gpu::Light> CollectLights(const Scene& scene);
}
