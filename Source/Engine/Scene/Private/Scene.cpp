#include "Engine/Scene/Scene.hpp"

#include "Engine/Engine.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Components/EnvironmentComponent.hpp"
#include "Engine/Scene/GlobalIllumination.hpp"
#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Scene/SceneLoader.hpp"

namespace Details
{
    StorageRange GetStorageRange(Scene& srcScene, Scene& dstScene)
    {
        StorageRange storageRange;

        const auto& dstTsc = dstScene.ctx().get<TextureStorageComponent>();
        const auto& dstMsc = dstScene.ctx().get<MaterialStorageComponent>();
        const auto& dstGsc = dstScene.ctx().get<GeometryStorageComponent>();

        storageRange.textures.offset = static_cast<uint32_t>(dstTsc.textures.size());
        storageRange.materials.offset = static_cast<uint32_t>(dstMsc.materials.size());
        storageRange.primitives.offset = static_cast<uint32_t>(dstGsc.primitives.size());

        const auto& srcTsc = srcScene.ctx().get<TextureStorageComponent>();
        const auto& srcMsc = srcScene.ctx().get<MaterialStorageComponent>();
        const auto& srcGsc = srcScene.ctx().get<GeometryStorageComponent>();

        storageRange.textures.size = static_cast<uint32_t>(srcTsc.textures.size());
        storageRange.materials.size = static_cast<uint32_t>(srcMsc.materials.size());
        storageRange.primitives.size = static_cast<uint32_t>(srcGsc.primitives.size());

        return storageRange;
    }

    void ApplyStorageOffsets(Scene& scene, const StorageRange& range)
    {
        for (auto& material : scene.ctx().get<MaterialStorageComponent>().materials)
        {
            const int32_t offset = static_cast<int32_t>(range.viewSamplers.offset);

            MaterialHelpers::ApplyTextureOffset(material, offset);
        }

        for (auto&& [entity, rc] : scene.view<RenderComponent>().each())
        {
            for (auto& ro : rc.renderObjects)
            {
                ro.primitive += range.primitives.offset;
                ro.material += range.materials.offset;
            }
        }
    }

    void RemoveStorageOffsets(Scene& scene, const StorageRange& range)
    {
        for (auto& material : scene.ctx().get<MaterialStorageComponent>().materials)
        {
            const int32_t offset = static_cast<int32_t>(range.viewSamplers.offset);

            MaterialHelpers::RemoveTextureOffset(material, offset);
        }

        for (auto&& [entity, rc] : scene.view<RenderComponent>().each())
        {
            for (auto& ro : rc.renderObjects)
            {
                if (ro.primitive >= range.primitives.offset)
                {
                    ro.primitive -= range.primitives.offset;
                }
                if (ro.material >= range.materials.offset)
                {
                    ro.material -= range.materials.offset;
                }
            }
        }
    }
}

Scene::Scene() = default;

Scene::Scene(const Filepath& path)
{
    SceneLoader sceneLoader(*this, path);
}

Scene::~Scene()
{
    for (const auto&& [entity, ec] : view<EnvironmentComponent>().each())
    {
        ResourceContext::DestroyResourceDelayed(ec.cubemapTexture.image);
        ResourceContext::DestroyResourceDelayed(ec.irradianceTexture.image);
        ResourceContext::DestroyResourceDelayed(ec.reflectionTexture.image);
    }

    for (const auto&& [entity, lvc] : view<LightVolumeComponent>().each())
    {
        ResourceContext::DestroyResourceDelayed(lvc.coefficientsBuffer);
        ResourceContext::DestroyResourceDelayed(lvc.tetrahedralBuffer);
        ResourceContext::DestroyResourceDelayed(lvc.positionsBuffer);
    }

    if (const auto* tsc = ctx().find<TextureStorageComponent>())
    {
        for (const auto& [image, sampler] : tsc->textures)
        {
            TextureCache::ReleaseTexture(image);
        }

        TextureCache::DestroyUnusedTextures();
    }
}

void Scene::EnumerateDescendants(entt::entity entity, const SceneEntityFunc& func) const
{
    if (entity != entt::null)
    {
        for (const auto child : get<HierarchyComponent>(entity).GetChildren())
        {
            func(child);

            EnumerateDescendants(child, func);
        }
    }
    else
    {
        each(func);
    }
}

void Scene::EnumerateAncestors(entt::entity entity, const SceneEntityFunc& func) const
{
    Assert(entity != entt::null);

    const entt::entity parent = get<HierarchyComponent>(entity).GetParent();

    if (parent != entt::null)
    {
        func(parent);

        EnumerateAncestors(parent, func);
    }
}

void Scene::EnumerateRenderView(const SceneRenderFunc& func) const
{
    for (auto&& [entity, tc, rc] : view<TransformComponent, RenderComponent>().each())
    {
        for (const auto ro : rc.renderObjects)
        {
            func(tc.GetWorldTransform(), ro);
        }
    }
}

entt::entity Scene::FindEntity(const std::string& name) const
{
    for (auto&& [entity, nc] : view<NameComponent>().each())
    {
        if (nc.name == name)
        {
            return entity;
        }
    }

    return entt::null;
}

entt::entity Scene::CreateEntity(entt::entity parent, const Transform& transform)
{
    const entt::entity entity = create();

    emplace<HierarchyComponent>(entity, *this, entity, parent);

    emplace<TransformComponent>(entity, *this, entity, transform);

    return entity;
}

entt::entity Scene::CloneEntity(entt::entity entity, const Transform& transform)
{
    const entt::entity clonedEntity = CreateEntity(get<HierarchyComponent>(entity).GetParent(), transform);

    SceneHelpers::CopyComponents(*this, *this, entity, clonedEntity);

    SceneHelpers::CopyHierarchy(*this, *this, entity, clonedEntity);

    return clonedEntity;
}

const Transform& Scene::GetEntityTransform(entt::entity entity) const
{
    return get<TransformComponent>(entity).GetWorldTransform();
}

void Scene::RemoveEntity(entt::entity entity)
{
    if (try_get<ScenePrefabComponent>(entity))
    {
        EraseScenePrefab(entity);
    }

    if (const auto* sic = try_get<SceneInstanceComponent>(entity))
    {
        std::erase(get<ScenePrefabComponent>(sic->prefab).instances, sic->prefab);
    }

    get<HierarchyComponent>(entity).SetParent(entt::null);

    RemoveChildren(entity);

    destroy(entity);
}

void Scene::RemoveChildren(entt::entity entity)
{
    const std::vector<entt::entity> children = get<HierarchyComponent>(entity).GetChildren();

    for (const auto child : children)
    {
        RemoveEntity(child);
    }
}

void Scene::EmplaceScenePrefab(Scene&& scene, entt::entity entity)
{
    auto& spc = emplace<ScenePrefabComponent>(entity);

    spc.storageRange = Details::GetStorageRange(scene, *this);

    spc.hierarchy = std::make_unique<Scene>();

    Details::ApplyStorageOffsets(scene, spc.storageRange);

    SceneHelpers::CopyHierarchy(scene, *spc.hierarchy, entt::null, entt::null);

    SceneHelpers::MergeStorageComponents(scene, *this);
}

void Scene::EmplaceSceneInstance(entt::entity scene, entt::entity entity)
{
    emplace<SceneInstanceComponent>(entity, scene);

    auto& spc = get<ScenePrefabComponent>(scene);

    SceneHelpers::CopyHierarchy(*spc.hierarchy, *this, entt::null, entity);

    spc.instances.push_back(entity);
}

entt::entity Scene::CreateSceneInstance(entt::entity scene, const Transform& transform)
{
    const entt::entity entity = CreateEntity(entt::null, {});

    EmplaceSceneInstance(scene, entity);

    for (const auto child : get<HierarchyComponent>(entity).GetChildren())
    {
        auto& tc = get<TransformComponent>(child);

        tc.SetLocalTransform(tc.GetLocalTransform() * transform);
    }

    return entity;
}

std::unique_ptr<Scene> Scene::EraseScenePrefab(entt::entity scene)
{
    auto spc = std::move(get<ScenePrefabComponent>(scene));

    SceneHelpers::SplitStorageComponents(*this, *spc.hierarchy, spc.storageRange);

    Details::RemoveStorageOffsets(*spc.hierarchy, spc.storageRange);

    Details::RemoveStorageOffsets(*this, spc.storageRange);

    for (const auto instance : spc.instances)
    {
        RemoveEntity(instance);
    }

    remove<ScenePrefabComponent>(scene);

    return std::move(spc.hierarchy);
}
