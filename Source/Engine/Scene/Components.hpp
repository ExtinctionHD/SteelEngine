#pragma once

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

};

namespace ComponentHelpers
{
    void AccumulateTransform(Scene& scene, entt::entity entity);
}