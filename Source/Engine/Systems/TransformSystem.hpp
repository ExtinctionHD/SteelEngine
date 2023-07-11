#pragma once

#include "Engine/Systems/System.hpp"

class Scene;
class TransformComponent;
struct HierarchyComponent;

class TransformSystem
        : public System
{
public:
    void Process(Scene& scene, float deltaSeconds) override;

private:
    static void UpdateTransform(Scene& scene, entt::entity entity, TransformComponent& tc);

    static void UpdateChildrenTransform(Scene& scene, const HierarchyComponent& hc);
};
