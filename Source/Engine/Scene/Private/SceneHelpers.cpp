#include "Engine/Scene/SceneHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Components/EnvironmentComponent.hpp"
#include "Engine/Scene/Components/AnimationComponent.hpp"
#include "Engine/Scene/Scene.hpp"

#include "Utils/Assert.hpp"

namespace Details
{
    using EntityMap = std::map<entt::entity, entt::entity>;

    template <class T>
    static void MoveRange(std::vector<T>& src, std::vector<T>& dst, const Range& range)
    {
        const auto srcBegin = src.begin() + range.offset;
        const auto srcEnd = src.begin() + range.offset + range.size;

        std::move(srcBegin, srcEnd, std::back_inserter(dst));

        src.erase(srcBegin, srcEnd);
    }

    std::vector<entt::entity> GetParentHierarchy(entt::entity entity, const Scene& scene)
    {
        std::vector<entt::entity> hierarchy;

        hierarchy.push_back(entity);

        entt::entity parent = entity;

        while (parent != entt::null)
        {
            parent = scene.get<HierarchyComponent>(parent).GetParent();
            hierarchy.push_back(parent);
        }

        return hierarchy;
    }

    void CopyAnimationComponent(
            const AnimationComponent& srcAc, AnimationComponent& dstAc, const EntityMap& entityMap)
    {
        dstAc = srcAc;

        for (auto& animation : dstAc.animations)
        {
            for (auto& track : animation.tracks)
            {
                track.target = entityMap.at(track.target);
            }
        }
    }

    void CopyRootAnimationComponent(const Scene& srcScene, Scene& dstScene,
            entt::entity srcParent, entt::entity dstParent, const EntityMap& entityMap)
    {
        const AnimationComponent* srcAc;

        if (srcParent != entt::null)
        {
            srcAc = srcScene.try_get<AnimationComponent>(srcParent);
        }
        else
        {
            srcAc = srcScene.ctx().find<AnimationComponent>();
        }

        if (srcAc)
        {
            AnimationComponent* dstAc;

            if (dstParent != entt::null)
            {
                dstAc = &dstScene.emplace<AnimationComponent>(dstParent);
            }
            else
            {
                dstAc = &dstScene.ctx().emplace<AnimationComponent>();
            }

            Details::CopyAnimationComponent(*srcAc, *dstAc, entityMap);
        }
    }

    void CopyComponents(const Scene& srcScene, Scene& dstScene,
            entt::entity srcEntity, entt::entity dstEntity, const EntityMap& entityMap)
    {
        if (const auto* nc = srcScene.try_get<NameComponent>(srcEntity))
        {
            dstScene.emplace<NameComponent>(dstEntity) = *nc;
        }
        if (const auto* rc = srcScene.try_get<RenderComponent>(srcEntity))
        {
            dstScene.emplace<RenderComponent>(dstEntity) = *rc;
        }
        if (const auto* cc = srcScene.try_get<CameraComponent>(srcEntity))
        {
            dstScene.emplace<CameraComponent>(dstEntity) = *cc;
        }
        if (const auto* lc = srcScene.try_get<LightComponent>(srcEntity))
        {
            dstScene.emplace<LightComponent>(dstEntity) = *lc;
        }
        if (const auto* ec = srcScene.try_get<EnvironmentComponent>(srcEntity))
        {
            dstScene.emplace<EnvironmentComponent>(dstEntity) = *ec;
        }
        if (const auto* ac = srcScene.try_get<AnimationComponent>(srcEntity))
        {
            CopyAnimationComponent(*ac, dstScene.emplace<AnimationComponent>(dstEntity), entityMap);
        }
    }
}

AABBox SceneHelpers::ComputeBBox(const Scene& scene)
{
    AABBox bbox;

    const auto& geometryStorageComponent = scene.ctx().get<GeometryStorageComponent>();

    for (auto&& [entity, tc, rc] : scene.view<TransformComponent, RenderComponent>().each())
    {
        for (const auto& ro : rc.renderObjects)
        {
            const Primitive& primitive = geometryStorageComponent.primitives[ro.primitive];

            bbox.Add(primitive.GetBBox().GetTransformed(tc.GetWorldTransform().GetMatrix()));
        }
    }

    return bbox;
}

std::string SceneHelpers::GetDefaultName(entt::entity entity)
{
    return std::format("entity_{}", static_cast<entt::id_type>(entity));
}

std::string SceneHelpers::GetDisplayName(const Scene& scene, entt::entity entity)
{
    if (auto* nc = scene.try_get<NameComponent>(entity))
    {
        return nc->name;
    }

    return GetDefaultName(entity);
}

bool SceneHelpers::IsChild(const Scene& scene, entt::entity entity, entt::entity parent)
{
    entt::entity currentParent = scene.get<HierarchyComponent>(entity).GetParent();

    while (currentParent != entt::null && currentParent != parent)
    {
        currentParent = scene.get<HierarchyComponent>(currentParent).GetParent();
    }

    return currentParent == parent;
}

entt::entity SceneHelpers::FindCommonParent(const Scene& scene, const std::set<entt::entity>& entities)
{
    if (entities.empty())
    {
        return entt::null;
    }

    if (entities.size() == 1)
    {
        return *entities.begin();
    }

    std::vector<entt::entity> commonHierarchy = Details::GetParentHierarchy(*entities.begin(), scene);

    for (auto it = std::next(entities.begin()); it != entities.end(); ++it)
    {
        const std::vector<entt::entity> otherHierarchy = Details::GetParentHierarchy(*it, scene);

        const auto difference = std::ranges::remove_if(commonHierarchy, [&](entt::entity entity)
            {
                return std::ranges::find(otherHierarchy, entity) == otherHierarchy.end();
            });

        commonHierarchy.erase(difference.begin(), difference.end());
    }

    if (!commonHierarchy.empty())
    {
        return commonHierarchy.front();
    }

    return entt::null;
}

void SceneHelpers::CopyHierarchy(
        const Scene& srcScene, Scene& dstScene, entt::entity srcParent, entt::entity dstParent)
{
    Details::EntityMap entityMap;

    srcScene.EnumerateDescendants(srcParent, [&](const entt::entity srcEntity)
        {
            const auto& srcTc = srcScene.get<TransformComponent>(srcEntity);

            entityMap.emplace(srcEntity, dstScene.CreateEntity(entt::null, srcTc.GetLocalTransform()));
        });

    for (const auto& [srcEntity, dstEntity] : entityMap)
    {
        const auto& srcHc = srcScene.get<HierarchyComponent>(srcEntity);

        if (srcHc.GetParent() == srcParent)
        {
            dstScene.get<HierarchyComponent>(dstEntity).SetParent(dstParent);
        }
        else
        {
            dstScene.get<HierarchyComponent>(dstEntity).SetParent(entityMap.at(srcHc.GetParent()));
        }

        Details::CopyComponents(srcScene, dstScene, srcEntity, dstEntity, entityMap);
    }

    Details::CopyRootAnimationComponent(srcScene, dstScene, srcParent, dstParent, entityMap);
}

void SceneHelpers::MergeStorageComponents(Scene& srcScene, Scene& dstScene)
{
    auto& srcTsc = srcScene.ctx().get<TextureStorageComponent>();
    auto& dstTsc = dstScene.ctx().get<TextureStorageComponent>();

    dstTsc.updated = !srcTsc.textures.empty();

    std::ranges::move(srcTsc.textures, std::back_inserter(dstTsc.textures));

    srcScene.ctx().erase<TextureStorageComponent>();

    auto& srcMsc = srcScene.ctx().get<MaterialStorageComponent>();
    auto& dstMsc = dstScene.ctx().get<MaterialStorageComponent>();

    dstMsc.updated = !srcMsc.materials.empty();

    std::ranges::move(srcMsc.materials, std::back_inserter(dstMsc.materials));

    srcScene.ctx().erase<MaterialStorageComponent>();

    auto& srcGsc = srcScene.ctx().get<GeometryStorageComponent>();
    auto& dstGsc = dstScene.ctx().get<GeometryStorageComponent>();

    dstGsc.updated = !srcGsc.primitives.empty();

    std::ranges::move(srcGsc.primitives, std::back_inserter(dstGsc.primitives));

    srcScene.ctx().erase<GeometryStorageComponent>();
}

void SceneHelpers::SplitStorageComponents(Scene& srcScene, Scene& dstScene, const StorageRange& range)
{
    auto& srcTsc = srcScene.ctx().get<TextureStorageComponent>();
    auto& dstTsc = dstScene.ctx().emplace<TextureStorageComponent>();

    srcTsc.updated = range.textures.size > 0;
    dstTsc.updated = range.textures.size > 0;

    Details::MoveRange(srcTsc.textures, dstTsc.textures, range.textures);

    auto& srcMsc = srcScene.ctx().get<MaterialStorageComponent>();
    auto& dstMsc = dstScene.ctx().emplace<MaterialStorageComponent>();

    srcMsc.updated = range.materials.size > 0;
    dstMsc.updated = range.materials.size > 0;

    Details::MoveRange(srcMsc.materials, dstMsc.materials, range.materials);

    auto& srcGsc = srcScene.ctx().get<GeometryStorageComponent>();
    auto& dstGsc = dstScene.ctx().emplace<GeometryStorageComponent>();

    srcGsc.updated = range.primitives.size > 0;
    dstGsc.updated = range.primitives.size > 0;

    Details::MoveRange(srcGsc.primitives, dstGsc.primitives, range.primitives);
}

vk::AccelerationStructureInstanceKHR SceneHelpers::GetTlasInstance(
        const Scene& scene, const TransformComponent& tc, const RenderObject& ro)
{
    const auto& geometryComponent = scene.ctx().get<GeometryStorageComponent>();
    const auto& materialComponent = scene.ctx().get<MaterialStorageComponent>();

    vk::TransformMatrixKHR transformMatrix;

    const glm::mat4 transposedTransform = glm::transpose(tc.GetWorldTransform().GetMatrix());

    std::memcpy(&transformMatrix.matrix, &transposedTransform, sizeof(vk::TransformMatrixKHR));

    Assert(ro.primitive <= static_cast<uint32_t>(std::numeric_limits<uint16_t>::max()));
    Assert(ro.material <= static_cast<uint32_t>(std::numeric_limits<uint8_t>::max()));

    const uint32_t customIndex = ro.primitive | (ro.material << 16);

    const Material& material = materialComponent.materials[ro.material];

    const vk::GeometryInstanceFlagsKHR flags = MaterialHelpers::GetTlasInstanceFlags(material.flags);

    const vk::AccelerationStructureKHR blas = geometryComponent.primitives[ro.primitive].GetBlas();

    return vk::AccelerationStructureInstanceKHR(transformMatrix,
            customIndex, 0xFF, 0, flags, VulkanContext::device->GetAddress(blas));
}
