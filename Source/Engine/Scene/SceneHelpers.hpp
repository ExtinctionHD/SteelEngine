#pragma once

#include "Utils/AABBox.hpp"
#include "Utils/Helpers.hpp"

class Scene;
class TransformComponent;
class Transform;
struct RenderObject;

struct StorageRange
{
    Range textures;
    Range samplers;
    Range textureSamplers;

    Range materials;
    Range primitives;
};

using SceneEntityFunc = std::function<void(entt::entity)>;

using SceneRenderFunc = std::function<void(const Transform&, const RenderObject&)>;

namespace SceneHelpers
{
    AABBox ComputeBBox(const Scene& scene);

    bool IsChild(const Scene& scene, entt::entity entity, entt::entity parent);

    void CopyComponents(const Scene& srcScene, Scene& dstScene, entt::entity srcEntity, entt::entity dstEntity);

    void CopyHierarchy(const Scene& srcScene, Scene& dstScene, entt::entity srcParent, entt::entity dstParent);

    void MergeStorageComponents(Scene& srcScene, Scene& dstScene);

    void SplitStorageComponents(Scene& srcScene, Scene& dstScene, const StorageRange& range);

    vk::AccelerationStructureInstanceKHR GetTlasInstance(
            const Scene& scene, const TransformComponent& tc, const RenderObject& ro);
}
