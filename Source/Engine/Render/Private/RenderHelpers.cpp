#include "Engine/Render/RenderHelpers.hpp"

#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/SceneRenderer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/Pipelines/MaterialPipelineCache.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"
#include "Engine/Scene/Components/EnvironmentComponent.hpp"
#include "Engine/Scene/GlobalIllumination.hpp"
#include "Engine/Scene/ImageBasedLighting.hpp"
#include "Engine/Scene/Scene.hpp"

void RenderHelpers::PushEnvironmentDescriptorData(DescriptorProvider& descriptorProvider, const Scene& scene)
{
    const auto& environmentComponent = scene.ctx().get<EnvironmentComponent>();

    const ImageBasedLighting& imageBasedLighting = *RenderContext::imageBasedLighting;

    descriptorProvider.PushGlobalData("irradianceMap", &environmentComponent.irradianceTexture);
    descriptorProvider.PushGlobalData("reflectionMap", &environmentComponent.reflectionTexture);
    descriptorProvider.PushGlobalData("specularLut", &imageBasedLighting.GetSpecularLut());
}

void RenderHelpers::PushLightVolumeDescriptorData(DescriptorProvider& descriptorProvider, const Scene& scene)
{
    if (scene.ctx().contains<LightVolumeComponent>())
    {
        const auto& lightVolumeComponent = scene.ctx().get<LightVolumeComponent>();

        descriptorProvider.PushGlobalData("positions", lightVolumeComponent.positionsBuffer);
        descriptorProvider.PushGlobalData("tetrahedral", lightVolumeComponent.tetrahedralBuffer);
        descriptorProvider.PushGlobalData("coefficients", lightVolumeComponent.coefficientsBuffer);
    }
}

void RenderHelpers::PushRayTracingDescriptorData(DescriptorProvider& descriptorProvider, const Scene& scene,
        const TopLevelAS& tlas)
{
    Assert(RenderOptions::rayTracingAllowed);

    const auto& geometryComponent = scene.ctx().get<GeometryStorageComponent>();

    std::vector<vk::Buffer> indexBuffers;
    std::vector<vk::Buffer> texCoordBuffers;

    indexBuffers.reserve(geometryComponent.primitives.size());
    texCoordBuffers.reserve(geometryComponent.primitives.size());

    for (const auto& primitive : geometryComponent.primitives)
    {
        indexBuffers.push_back(primitive.GetIndexBuffer());
        texCoordBuffers.push_back(primitive.GetTexCoordBuffer());
    }

    descriptorProvider.PushGlobalData("tlas", &tlas);
    descriptorProvider.PushGlobalData("indexBuffers", &indexBuffers);
    descriptorProvider.PushGlobalData("texCoordBuffers", &texCoordBuffers);
}

std::set<MaterialFlags> RenderHelpers::CacheMaterialPipelines(const Scene& scene,
        MaterialPipelineCache& cache, const MaterialPipelinePred& pred)
{
    std::set<MaterialFlags> uniquePipelines;

    const auto& materialComponent = scene.ctx().get<MaterialStorageComponent>();

    for (const auto& material : materialComponent.materials)
    {
        if (pred(material.flags))
        {
            cache.GetPipeline(material.flags);

            uniquePipelines.emplace(material.flags);
        }
    }

    return uniquePipelines;
}
