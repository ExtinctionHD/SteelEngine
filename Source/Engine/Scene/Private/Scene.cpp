#include "Engine/Scene/Scene.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/SceneComponents.hpp"
#include "Engine/Scene/Components.hpp"
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

    vk::Buffer CreateMaterialBuffer(const Scene& scene)
    {
        const SceneMaterialComponent& smc = scene.ctx().at<SceneMaterialComponent>();

        std::vector<gpu::Material> materialData;
        materialData.reserve(smc.materials.size());

        for (const Material& material : smc.materials)
        {
            materialData.push_back(material.data);
        }

        return BufferHelpers::CreateBufferWithData(
                vk::BufferUsageFlagBits::eUniformBuffer, ByteView(materialData));
    }

    vk::AccelerationStructureKHR GenerateTlas(const Scene& scene)
    {
        const SceneRayTracingComponent& srtc = scene.ctx().at<SceneRayTracingComponent>();
        const SceneMaterialComponent& smc = scene.ctx().at<SceneMaterialComponent>();

        std::vector<TlasInstanceData> instances;
        for (auto&& [entity, tc, rc] : scene.view<TransformComponent, RenderComponent>().each())
        {
            for (const auto& ro : rc.renderObjects)
            {
                Assert(ro.primitive <= static_cast<uint32_t>(std::numeric_limits<uint16_t>::max()));
                Assert(ro.material <= static_cast<uint32_t>(std::numeric_limits<uint8_t>::max()));

                const Material& material = smc.materials[ro.material];

                const vk::AccelerationStructureKHR blas = srtc.blases[ro.primitive];

                const vk::GeometryInstanceFlagsKHR flags = MaterialHelpers::GetTlasInstanceFlags(material.flags);

                TlasInstanceData instance;
                instance.blas = blas;
                instance.transform = tc.worldTransform;
                instance.customIndex = ro.primitive | (ro.material << 16);
                instance.mask = 0xFF;
                instance.sbtRecordOffset = 0;
                instance.flags = flags;

                instances.push_back(instance);
            }
        }

        return VulkanContext::accelerationStructureManager->GenerateTlas(instances);
    }
}

class SceneAdder
{
public:
    SceneAdder(Scene&& srcScene_, Scene& dstScene_, entt::entity parent_)
        : srcScene(std::move(srcScene_))
        , dstScene(dstScene_)
        , parent(parent_)
    {
        const auto& stc = dstScene.ctx().at<SceneTextureComponent>();
        const auto& smc = dstScene.ctx().at<SceneMaterialComponent>();
        const auto& sgc = dstScene.ctx().at<SceneGeometryComponent>();
        
        textureOffset = static_cast<int32_t>(stc.textures.size());
        materialOffset = static_cast<uint32_t>(smc.materials.size());
        primitiveOffset = static_cast<uint32_t>(sgc.primitives.size());

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
        }

        for (const auto& [srcEntity, dstEntity] : entities)
        {
            ComponentHelpers::AccumulateTransform(dstScene, dstEntity);
        }
        
        MergeSceneTextureComponents();

        MergeSceneMaterialsComponents();

        MergeSceneGeometryComponents();

        MergeSceneRayTracingComponent();
    }

private:
    Scene&& srcScene;
    Scene& dstScene;

    entt::entity parent;

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
        else
        {
            dstHc.parent = parent;
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

    void MergeSceneTextureComponents() const
    {
        auto& srcStc = srcScene.ctx().at<SceneTextureComponent>();
        auto& dstStc = dstScene.ctx().at<SceneTextureComponent>();

        std::ranges::move(srcStc.images, std::back_inserter(dstStc.images));
        std::ranges::move(srcStc.samplers, std::back_inserter(dstStc.samplers));
        std::ranges::move(srcStc.textures, std::back_inserter(dstStc.textures));
    }

    void MergeSceneMaterialsComponents() const
    {
        auto& srcSmc = srcScene.ctx().at<SceneMaterialComponent>();
        auto& dstSmc = dstScene.ctx().at<SceneMaterialComponent>();

        dstSmc.materials.reserve(dstSmc.materials.size() + srcSmc.materials.size());

        for (auto& srcMaterial : srcSmc.materials)
        {
            Details::AddTextureOffset(srcMaterial, textureOffset);

            dstSmc.materials.push_back(srcMaterial);
        }
    }

    void MergeSceneGeometryComponents() const
    {
        auto& srcSgc = srcScene.ctx().at<SceneGeometryComponent>();
        auto& dstSgc = dstScene.ctx().at<SceneGeometryComponent>();

        std::ranges::move(srcSgc.primitives, std::back_inserter(dstSgc.primitives));
    }

    void MergeSceneRayTracingComponent() const
    {
        auto& srcSrtc = srcScene.ctx().at<SceneRayTracingComponent>();
        auto& dstSrtc = dstScene.ctx().at<SceneRayTracingComponent>();
        
        std::ranges::move(srcSrtc.indexBuffers, std::back_inserter(dstSrtc.indexBuffers));
        std::ranges::move(srcSrtc.vertexBuffers, std::back_inserter(dstSrtc.vertexBuffers));
        std::ranges::move(srcSrtc.blases, std::back_inserter(dstSrtc.blases));
    }
};

Scene::Scene(const Filepath& path)
{
    SceneHelpers::LoadScene(*this, path);
}

void Scene::AddScene(Scene&& scene, entt::entity parent)
{
    SceneAdder sceneAdder(std::move(scene), *this, parent);
}

void Scene::PrepareToRender()
{
    auto& src = ctx().emplace<SceneRenderComponent>();

    src.materialBuffer = Details::CreateMaterialBuffer(*this);
    src.tlas = Details::GenerateTlas(*this);
}

AABBox SceneHelpers::CalculateSceneBBox(const Scene& scene)
{
    AABBox bbox;

    const auto& geometryComponent = scene.ctx().at<SceneGeometryComponent>();

    for (auto&& [entity, tc, rc] : scene.view<TransformComponent, RenderComponent>().each())
    {
        for (const auto& ro : rc.renderObjects)
        {
            const Primitive& primitive = geometryComponent.primitives[ro.primitive];

            bbox.Add(primitive.bbox.GetTransformed(tc.worldTransform));
        }
    }

    return bbox;
}
