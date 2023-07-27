#pragma once

#include "Utils/AABBox.hpp"

class Scene;
class TransformComponent;
struct RenderObject;

namespace SceneHelpers
{
    AABBox ComputeSceneBBox(const Scene& scene);

    vk::AccelerationStructureInstanceKHR GetTlasInstance(const Scene& scene,
            const TransformComponent& tc, const RenderObject& ro);
}
