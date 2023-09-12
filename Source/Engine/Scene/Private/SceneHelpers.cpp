#include "Engine/Scene/SceneHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Components/EnvironmentComponent.hpp"
#include "Engine/Scene/Scene.hpp"

#include "Utils/Assert.hpp"

namespace Details
{
    template <class T>
    static void MoveRange(std::vector<T>& src, std::vector<T>& dst, const Range& range)
    {
        const auto srcBegin = src.begin() + range.offset;
        const auto srcEnd = src.begin() + range.offset + range.size;

        std::move(srcBegin, srcEnd, std::back_inserter(dst));

        src.erase(srcBegin, srcEnd);
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

bool SceneHelpers::IsChild(const Scene& scene, entt::entity entity, entt::entity parent)
{
    entt::entity currentParent = scene.get<HierarchyComponent>(entity).GetParent();

    while (currentParent != entt::null && currentParent != parent)
    {
        currentParent = scene.get<HierarchyComponent>(currentParent).GetParent();
    }

    return currentParent == parent;
}

void SceneHelpers::CopyComponents(
        const Scene& srcScene, Scene& dstScene, entt::entity srcEntity, entt::entity dstEntity)
{
    dstScene.emplace<TransformComponent>(dstEntity) = srcScene.get<TransformComponent>(srcEntity);

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
}

void SceneHelpers::CopyHierarchy(
        const Scene& srcScene, Scene& dstScene, entt::entity srcParent, entt::entity dstParent)
{
    std::map<entt::entity, entt::entity> entities;

    srcScene.EnumerateChildren(srcParent, [&](const entt::entity srcEntity)
        {
            entities.emplace(srcEntity, dstScene.create());
        });

    for (const auto& [srcEntity, dstEntity] : entities)
    {
        const auto& srcHc = srcScene.get<HierarchyComponent>(srcEntity);

        if (srcHc.GetParent() == srcParent)
        {
            dstScene.emplace<HierarchyComponent>(dstEntity, dstScene, dstEntity, dstParent);
        }
        else
        {
            dstScene.emplace<HierarchyComponent>(dstEntity, dstScene, dstEntity, entities.at(srcHc.GetParent()));
        }

        CopyComponents(srcScene, dstScene, srcEntity, dstEntity);
    }
}

void SceneHelpers::MergeStorageComponents(Scene& srcScene, Scene& dstScene)
{
    auto& srcTsc = srcScene.ctx().get<TextureStorageComponent>();
    auto& dstTsc = dstScene.ctx().get<TextureStorageComponent>();

    dstTsc.updated = !srcTsc.viewSamplers.empty();

    std::ranges::move(srcTsc.textures, std::back_inserter(dstTsc.textures));
    std::ranges::move(srcTsc.samplers, std::back_inserter(dstTsc.samplers));
    std::ranges::move(srcTsc.viewSamplers, std::back_inserter(dstTsc.viewSamplers));

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

    srcTsc.updated = range.viewSamplers.size > 0;
    dstTsc.updated = range.viewSamplers.size > 0;

    Details::MoveRange(srcTsc.textures, dstTsc.textures, range.textures);
    Details::MoveRange(srcTsc.samplers, dstTsc.samplers, range.samplers);
    Details::MoveRange(srcTsc.viewSamplers, dstTsc.viewSamplers, range.viewSamplers);

    auto& srcMsc = srcScene.ctx().get<MaterialStorageComponent>();
    auto& dstMsc = dstScene.ctx().emplace<MaterialStorageComponent>();

    srcTsc.updated = range.materials.size > 0;
    dstTsc.updated = range.materials.size > 0;

    Details::MoveRange(srcMsc.materials, dstMsc.materials, range.materials);

    auto& srcGsc = srcScene.ctx().get<GeometryStorageComponent>();
    auto& dstGsc = dstScene.ctx().emplace<GeometryStorageComponent>();

    srcTsc.updated = range.primitives.size > 0;
    dstTsc.updated = range.primitives.size > 0;

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
