#include "Engine/Scene/SceneMerger.hpp"

#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/Components.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Scene/StorageComponents.hpp"

namespace Details
{
    void AddTextureOffset(Material& material, int32_t offset)
    {
        if (material.data.baseColorTexture >= 0)
        {
            material.data.baseColorTexture += offset;
        }
        if (material.data.roughnessMetallicTexture >= 0)
        {
            material.data.roughnessMetallicTexture += offset;
        }
        if (material.data.normalTexture >= 0)
        {
            material.data.normalTexture += offset;
        }
        if (material.data.occlusionTexture >= 0)
        {
            material.data.occlusionTexture += offset;
        }
        if (material.data.emissionTexture >= 0)
        {
            material.data.emissionTexture += offset;
        }
    }
}

SceneMerger::SceneMerger(Scene&& srcScene_, Scene& dstScene_, entt::entity dstSpawn_)
    : srcScene(std::move(srcScene_))
    , dstScene(dstScene_)
    , dstSpawn(dstSpawn_)
{
    const auto& tsc = dstScene.ctx().get<TextureStorageComponent>();
    const auto& msc = dstScene.ctx().get<MaterialStorageComponent>();
    const auto& gsc = dstScene.ctx().get<GeometryStorageComponent>();

    textureOffset = static_cast<int32_t>(tsc.textures.size());
    materialOffset = static_cast<uint32_t>(msc.materials.size());
    primitiveOffset = static_cast<uint32_t>(gsc.primitives.size());

    srcScene.each([&](const entt::entity srcEntity)
        {
            entities.emplace(srcEntity, dstScene.create());
        });

    for (const auto& [srcEntity, dstEntity] : entities)
    {
        AddHierarchyComponent(srcEntity, dstEntity);

        AddTransformComponent(srcEntity, dstEntity);

        if (srcScene.try_get<RenderComponent>(srcEntity))
        {
            AddRenderComponent(srcEntity, dstEntity);
        }

        if (srcScene.try_get<CameraComponent>(srcEntity))
        {
            AddCameraComponent(srcEntity, dstEntity);
        }

        if (srcScene.try_get<EnvironmentComponent>(srcEntity))
        {
            AddEnvironmentComponent(srcEntity, dstEntity);
        }
    }

    for (const auto& [srcEntity, dstEntity] : entities)
    {
        ComponentHelpers::AccumulateTransform(dstScene, dstEntity);
    }

    MergeTextureStorageComponents();

    MergeMaterialStorageComponents();

    MergeGeometryStorageComponents();

    MergeRayTracingStorageComponents();
}

void SceneMerger::AddHierarchyComponent(entt::entity srcEntity, entt::entity dstEntity) const
{
    const auto& srcHc = srcScene.get<HierarchyComponent>(srcEntity);

    auto& dstHc = dstScene.emplace<HierarchyComponent>(dstEntity);

    if (srcHc.parent != entt::null)
    {
        dstHc.parent = entities.at(srcHc.parent);
    }
    else if (dstSpawn != entt::null)
    {
        const auto& spawnHc = dstScene.get<HierarchyComponent>(dstSpawn);

        dstHc.parent = spawnHc.parent;

        if (dstHc.parent != entt::null)
        {
            auto& parentHc = dstScene.get<HierarchyComponent>(dstHc.parent);

            parentHc.children.push_back(dstEntity);
        }
    }

    dstHc.children.reserve(srcHc.children.size());

    for (entt::entity srcChild : srcHc.children)
    {
        dstHc.children.push_back(entities.at(srcChild));
    }
}

void SceneMerger::AddTransformComponent(entt::entity srcEntity, entt::entity dstEntity) const
{
    const auto& srcTc = srcScene.get<TransformComponent>(srcEntity);

    auto& dstTc = dstScene.emplace<TransformComponent>(dstEntity);

    dstTc.localTransform = srcTc.localTransform;

    if (dstSpawn != entt::null)
    {
        const auto& spawnTc = dstScene.get<TransformComponent>(dstSpawn);

        dstTc.localTransform = spawnTc.localTransform * dstTc.localTransform;
    }
}

void SceneMerger::AddRenderComponent(entt::entity srcEntity, entt::entity dstEntity) const
{
    const auto& srcRc = srcScene.get<RenderComponent>(srcEntity);

    auto& dstRc = dstScene.emplace<RenderComponent>(dstEntity);

    for (const auto& srcRo : srcRc.renderObjects)
    {
        RenderObject dstRo = srcRo;

        dstRo.primitive += primitiveOffset;
        dstRo.material += materialOffset;

        dstRc.renderObjects.push_back(dstRo);
    }
}

void SceneMerger::AddCameraComponent(entt::entity srcEntity, entt::entity dstEntity) const
{
    dstScene.emplace<CameraComponent>(dstEntity) = srcScene.get<CameraComponent>(srcEntity);
}

void SceneMerger::AddEnvironmentComponent(entt::entity srcEntity, entt::entity dstEntity) const
{
    dstScene.emplace<EnvironmentComponent>(dstEntity) = srcScene.get<EnvironmentComponent>(srcEntity);
}

void SceneMerger::MergeTextureStorageComponents() const
{
    auto& srcTsc = srcScene.ctx().get<TextureStorageComponent>();
    auto& dstTsc = dstScene.ctx().get<TextureStorageComponent>();

    std::ranges::move(srcTsc.images, std::back_inserter(dstTsc.images));
    std::ranges::move(srcTsc.samplers, std::back_inserter(dstTsc.samplers));
    std::ranges::move(srcTsc.textures, std::back_inserter(dstTsc.textures));
}

void SceneMerger::MergeMaterialStorageComponents() const
{
    auto& srcMsc = srcScene.ctx().get<MaterialStorageComponent>();
    auto& dstMsc = dstScene.ctx().get<MaterialStorageComponent>();

    dstMsc.materials.reserve(dstMsc.materials.size() + srcMsc.materials.size());

    for (auto& srcMaterial : srcMsc.materials)
    {
        Details::AddTextureOffset(srcMaterial, textureOffset);

        dstMsc.materials.push_back(srcMaterial);
    }
}

void SceneMerger::MergeGeometryStorageComponents() const
{
    auto& srcGsc = srcScene.ctx().get<GeometryStorageComponent>();
    auto& dstGsc = dstScene.ctx().get<GeometryStorageComponent>();

    std::ranges::move(srcGsc.primitives, std::back_inserter(dstGsc.primitives));
}

void SceneMerger::MergeRayTracingStorageComponents() const
{
    if constexpr (Config::kRayTracingEnabled)
    {
        auto& srcRtsc = srcScene.ctx().get<RayTracingStorageComponent>();
        auto& dstRtsc = dstScene.ctx().get<RayTracingStorageComponent>();

        std::ranges::move(srcRtsc.blases, std::back_inserter(dstRtsc.blases));
    }
}
