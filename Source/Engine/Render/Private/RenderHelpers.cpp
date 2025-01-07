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

vk::Rect2D RenderHelpers::GetSwapchainRenderArea()
{
    return vk::Rect2D(vk::Offset2D(), VulkanContext::swapchain->GetExtent());
}

vk::Viewport RenderHelpers::GetSwapchainViewport()
{
    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    return vk::Viewport(0.0f, 0.0f,
            static_cast<float>(extent.width),
            static_cast<float>(extent.height),
            0.0f, 1.0f);
}

void RenderHelpers::PushEnvironmentDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider)
{
    const auto& environmentComponent = scene.ctx().get<EnvironmentComponent>();

    const ImageBasedLighting& imageBasedLighting = *RenderContext::imageBasedLighting;

    descriptorProvider.PushGlobalData("irradianceMap", &environmentComponent.irradianceTexture);
    descriptorProvider.PushGlobalData("reflectionMap", &environmentComponent.reflectionTexture);
    descriptorProvider.PushGlobalData("specularLut", &imageBasedLighting.GetSpecularLut());
}

void RenderHelpers::PushLightVolumeDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider)
{
    if (scene.ctx().contains<LightVolumeComponent>())
    {
        const auto& lightVolumeComponent = scene.ctx().get<LightVolumeComponent>();

        descriptorProvider.PushGlobalData("positions", lightVolumeComponent.positionsBuffer);
        descriptorProvider.PushGlobalData("tetrahedral", lightVolumeComponent.tetrahedralBuffer);
        descriptorProvider.PushGlobalData("coefficients", lightVolumeComponent.coefficientsBuffer);
    }
}

void RenderHelpers::PushRayTracingDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider)
{
    Assert(RenderOptions::rayTracingAllowed);

    const auto& geometryComponent = scene.ctx().get<GeometryStorageComponent>();
    const auto& renderComponent = scene.ctx().get<RenderContextComponent>();

    std::vector<vk::Buffer> indexBuffers;
    std::vector<vk::Buffer> texCoordBuffers;

    indexBuffers.reserve(geometryComponent.primitives.size());
    texCoordBuffers.reserve(geometryComponent.primitives.size());

    for (const auto& primitive : geometryComponent.primitives)
    {
        indexBuffers.push_back(primitive.GetIndexBuffer());
        texCoordBuffers.push_back(primitive.GetTexCoordBuffer());
    }

    descriptorProvider.PushGlobalData("tlas", &renderComponent.tlas);
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
