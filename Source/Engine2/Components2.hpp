#pragma once

class Scene2;

struct TransformComponent
{
    glm::mat4 localTransform;
    glm::mat4 worldTransform;
};

struct HierarchyComponent
{
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
};

struct CameraComponent
{

};

struct EnvironmentComponent
{

};

struct LightComponent
{
    
};

namespace SceneHelpers
{
    glm::mat4 AccumulateTransform(const Scene2& scene, entt::entity entity);
}