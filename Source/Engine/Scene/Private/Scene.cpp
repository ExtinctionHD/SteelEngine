#include "Engine/Scene/Scene.hpp"

#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/StorageComponents.hpp"
#include "Engine/Scene/Components.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Scene/GlobalIllumination.hpp"
#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Primitive.hpp"
#include "Engine/Scene/SceneLoader.hpp"

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

    vk::Buffer CreateLightBuffer(const Scene& scene)
    {
        EASY_FUNCTION()

        if (scene.view<LightComponent>().empty())
        {
            return vk::Buffer();
        }

        const std::vector<gpu::Light> lights = ComponentHelpers::CollectLights(scene);

        return BufferHelpers::CreateBufferWithData(
                vk::BufferUsageFlagBits::eUniformBuffer, ByteView(lights));
    }

    vk::Buffer CreateMaterialBuffer(const Scene& scene)
    {
        EASY_FUNCTION()

        const auto& msc = scene.ctx().get<MaterialStorageComponent>();

        std::vector<gpu::Material> materialData;
        materialData.reserve(msc.materials.size());

        for (const Material& material : msc.materials)
        {
            materialData.push_back(material.data);
        }

        return BufferHelpers::CreateBufferWithData(
                vk::BufferUsageFlagBits::eUniformBuffer, ByteView(materialData));
    }

    vk::AccelerationStructureKHR GenerateTlas(const Scene& scene)
    {
        EASY_FUNCTION()

        const auto& rtsc = scene.ctx().get<RayTracingStorageComponent>();
        const auto& msc = scene.ctx().get<MaterialStorageComponent>();

        std::vector<TlasInstanceData> instances;
        for (auto&& [entity, tc, rc] : scene.view<TransformComponent, RenderComponent>().each())
        {
            for (const auto& ro : rc.renderObjects)
            {
                Assert(ro.primitive <= static_cast<uint32_t>(std::numeric_limits<uint16_t>::max()));
                Assert(ro.material <= static_cast<uint32_t>(std::numeric_limits<uint8_t>::max()));

                const Material& material = msc.materials[ro.material];

                const vk::AccelerationStructureKHR blas = rtsc.blases[ro.primitive];

                const vk::GeometryInstanceFlagsKHR flags = MaterialHelpers::GetTlasInstanceFlags(material.flags);

                TlasInstanceData instance;
                instance.blas = blas;
                instance.transform = tc.worldTransform.GetMatrix();
                instance.customIndex = ro.primitive | (ro.material << 16);
                instance.mask = 0xFF;
                instance.sbtRecordOffset = 0;
                instance.flags = flags;

                instances.push_back(instance);
            }
        }

        return VulkanContext::accelerationStructureManager->GenerateTlas(instances);
    }

    CameraComponent CreateDefaultCamera()
    {
        constexpr CameraLocation location = Config::DefaultCamera::kLocation;
        constexpr CameraProjection projection = Config::DefaultCamera::kProjection;

        const glm::mat4 viewMatrix = CameraHelpers::CalculateViewMatrix(location);
        const glm::mat4 projMatrix = CameraHelpers::CalculateProjMatrix(projection);

        return CameraComponent{ location, projection, viewMatrix, projMatrix };
    }

    EnvironmentComponent CreateDefaultEnvironment()
    {
        return EnvironmentHelpers::LoadEnvironment(Config::kDefaultPanoramaPath);
    }
}

class SceneAdder
{
public:
    SceneAdder(Scene&& srcScene_, Scene& dstScene_, entt::entity dstSpawn_)
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

private:
    Scene&& srcScene;
    Scene& dstScene;

    entt::entity dstSpawn;

    std::map<entt::entity, entt::entity> entities;

    int32_t textureOffset = 0;
    uint32_t materialOffset = 0;
    uint32_t primitiveOffset = 0;

    void AddHierarchyComponent(entt::entity srcEntity, entt::entity dstEntity) const
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

    void AddTransformComponent(entt::entity srcEntity, entt::entity dstEntity) const
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

    void AddRenderComponent(entt::entity srcEntity, entt::entity dstEntity) const
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

    void AddCameraComponent(entt::entity srcEntity, entt::entity dstEntity) const
    {
        dstScene.emplace<CameraComponent>(dstEntity) = srcScene.get<CameraComponent>(srcEntity);
    }

    void AddEnvironmentComponent(entt::entity srcEntity, entt::entity dstEntity) const
    {
        dstScene.emplace<EnvironmentComponent>(dstEntity) = srcScene.get<EnvironmentComponent>(srcEntity);
    }

    void MergeTextureStorageComponents() const
    {
        auto& srcTsc = srcScene.ctx().get<TextureStorageComponent>();
        auto& dstTsc = dstScene.ctx().get<TextureStorageComponent>();

        std::ranges::move(srcTsc.images, std::back_inserter(dstTsc.images));
        std::ranges::move(srcTsc.samplers, std::back_inserter(dstTsc.samplers));
        std::ranges::move(srcTsc.textures, std::back_inserter(dstTsc.textures));
    }

    void MergeMaterialStorageComponents() const
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

    void MergeGeometryStorageComponents() const
    {
        auto& srcGsc = srcScene.ctx().get<GeometryStorageComponent>();
        auto& dstGsc = dstScene.ctx().get<GeometryStorageComponent>();

        std::ranges::move(srcGsc.primitives, std::back_inserter(dstGsc.primitives));
    }

    void MergeRayTracingStorageComponents() const
    {
        if constexpr (Config::kRayTracingEnabled)
        {
            auto& srcRtsc = srcScene.ctx().get<RayTracingStorageComponent>();
            auto& dstRtsc = dstScene.ctx().get<RayTracingStorageComponent>();

            std::ranges::move(srcRtsc.indexBuffers, std::back_inserter(dstRtsc.indexBuffers));
            std::ranges::move(srcRtsc.vertexBuffers, std::back_inserter(dstRtsc.vertexBuffers));
            std::ranges::move(srcRtsc.blases, std::back_inserter(dstRtsc.blases));
        }
    }
};

Scene::Scene(const Filepath& path)
{
    SceneHelpers::LoadScene(*this, path);
}

Scene::~Scene()
{
    for (const auto [entity, ec] : view<EnvironmentComponent>().each())
    {
        VulkanContext::textureManager->DestroyTexture(ec.cubemapTexture);
        VulkanContext::textureManager->DestroyTexture(ec.irradianceTexture);
        VulkanContext::textureManager->DestroyTexture(ec.reflectionTexture);
    }

    for (const auto [entity, lvc] : view<LightVolumeComponent>().each())
    {
        VulkanContext::bufferManager->DestroyBuffer(lvc.coefficientsBuffer);
        VulkanContext::bufferManager->DestroyBuffer(lvc.tetrahedralBuffer);
        VulkanContext::bufferManager->DestroyBuffer(lvc.positionsBuffer);
    }

    const auto& tsc = ctx().get<TextureStorageComponent>();

    for (const Texture& texture : tsc.images)
    {
        VulkanContext::textureManager->DestroyTexture(texture);
    }

    for (const vk::Sampler sampler : tsc.samplers)
    {
        VulkanContext::textureManager->DestroySampler(sampler);
    }

    const auto& gsc = ctx().get<GeometryStorageComponent>();

    for (const Primitive& primitive : gsc.primitives)
    {
        VulkanContext::bufferManager->DestroyBuffer(primitive.vertexBuffer);
        VulkanContext::bufferManager->DestroyBuffer(primitive.indexBuffer);
    }

    if (const auto rtsc = ctx().find<RayTracingStorageComponent>())
    {
        for (const vk::Buffer buffer : rtsc->vertexBuffers)
        {
            VulkanContext::bufferManager->DestroyBuffer(buffer);
        }
        for (const vk::Buffer buffer : rtsc->indexBuffers)
        {
            VulkanContext::bufferManager->DestroyBuffer(buffer);
        }
        for (const vk::AccelerationStructureKHR blas : rtsc->blases)
        {
            VulkanContext::accelerationStructureManager->DestroyAccelerationStructure(blas);
        }
    }

    if (const auto rsc = ctx().find<RenderStorageComponent>())
    {
        if (rsc->lightBuffer)
        {
            VulkanContext::bufferManager->DestroyBuffer(rsc->lightBuffer);
        }
        if (rsc->materialBuffer)
        {
            VulkanContext::bufferManager->DestroyBuffer(rsc->materialBuffer);
        }
        if (rsc->tlas)
        {
            VulkanContext::accelerationStructureManager->DestroyAccelerationStructure(rsc->tlas);
        }
    }
}

void Scene::AddScene(Scene&& scene, entt::entity spawn)
{
    SceneAdder sceneAdder(std::move(scene), *this, spawn);
}

void Scene::PrepareToRender()
{
    EASY_FUNCTION()

    auto& rsc = ctx().emplace<RenderStorageComponent>();

    rsc.lightBuffer = Details::CreateLightBuffer(*this);

    rsc.materialBuffer = Details::CreateMaterialBuffer(*this);

    if constexpr (Config::kRayTracingEnabled)
    {
        rsc.tlas = Details::GenerateTlas(*this);
    }

    if (!ctx().contains<CameraComponent&>())
    {
        const entt::entity entity = create();

        auto& cc = emplace<CameraComponent>(entity);

        cc = Details::CreateDefaultCamera();

        ctx().emplace<CameraComponent&>(cc);
    }

    if (!ctx().contains<EnvironmentComponent&>())
    {
        const entt::entity entity = create();

        auto& ec = emplace<EnvironmentComponent>(entity);

        ec = Details::CreateDefaultEnvironment();

        ctx().emplace<EnvironmentComponent&>(ec);
    }

    if constexpr (Config::kGlobalIlluminationEnabled)
    {
        const entt::entity entity = create();

        auto& lvc = emplace<LightVolumeComponent>(entity);

        lvc = RenderContext::globalIllumination->GenerateLightVolume(*this);

        ctx().emplace<LightVolumeComponent&>(lvc);
    }
}

AABBox SceneHelpers::CalculateSceneBBox(const Scene& scene)
{
    AABBox bbox;

    const auto& geometryStorageComponent = scene.ctx().get<GeometryStorageComponent>();

    for (auto&& [entity, tc, rc] : scene.view<TransformComponent, RenderComponent>().each())
    {
        for (const auto& ro : rc.renderObjects)
        {
            const Primitive& primitive = geometryStorageComponent.primitives[ro.primitive];

            bbox.Add(primitive.bbox.GetTransformed(tc.worldTransform.GetMatrix()));
        }
    }

    return bbox;
}
