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
#include "Engine/Scene/SceneMerger.hpp"

namespace Details
{
    vk::Buffer CreateLightBuffer(const Scene& scene)
    {
        EASY_FUNCTION()

        if (scene.view<LightComponent>().empty())
        {
            return vk::Buffer();
        }

        const std::vector<gpu::Light> lights = ComponentHelpers::CollectLights(scene);

        return BufferHelpers::CreateBufferWithData(
                vk::BufferUsageFlagBits::eUniformBuffer, GetByteView(lights));
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
                vk::BufferUsageFlagBits::eUniformBuffer, GetByteView(materialData));
    }

    std::vector<vk::Buffer> CreateFrameBuffers()
    {
        std::vector<vk::Buffer> frameBuffers(VulkanContext::swapchain->GetImageCount());

        for (auto& frameBuffer : frameBuffers)
        {
            frameBuffer = BufferHelpers::CreateEmptyBuffer(
                    vk::BufferUsageFlagBits::eUniformBuffer, sizeof(gpu::Frame));
        }

        return frameBuffers;
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

        const glm::mat4 viewMatrix = CameraHelpers::ComputeViewMatrix(location);
        const glm::mat4 projMatrix = CameraHelpers::ComputeProjMatrix(projection);

        return CameraComponent{ location, projection, viewMatrix, projMatrix };
    }

    EnvironmentComponent CreateDefaultEnvironment()
    {
        return EnvironmentHelpers::LoadEnvironment(Config::kDefaultPanoramaPath);
    }
}

Scene::Scene(const Filepath& path)
{
    SceneLoader(*this, path);
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

    for (const Texture& texture : tsc.textures)
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
        PrimitiveHelpers::DestroyPrimitiveBuffers(primitive);
    }

    if (const auto rtsc = ctx().find<RayTracingStorageComponent>())
    {
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
        for (const auto& frameBuffer : rsc->frameBuffers)
        {
            VulkanContext::bufferManager->DestroyBuffer(frameBuffer);
        }
        if (rsc->tlas)
        {
            VulkanContext::accelerationStructureManager->DestroyAccelerationStructure(rsc->tlas);
        }
    }
}

void Scene::AddScene(Scene&& scene, entt::entity spawn)
{
    SceneMerger sceneAdder(std::move(scene), *this, spawn);
}

void Scene::PrepareToRender()
{
    EASY_FUNCTION()

    auto& rsc = ctx().emplace<RenderStorageComponent>();

    rsc.lightBuffer = Details::CreateLightBuffer(*this);

    rsc.materialBuffer = Details::CreateMaterialBuffer(*this);

    rsc.frameBuffers = Details::CreateFrameBuffers();

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

AABBox SceneHelpers::ComputeSceneBBox(const Scene& scene)
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
