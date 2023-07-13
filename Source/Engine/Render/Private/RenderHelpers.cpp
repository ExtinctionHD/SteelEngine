#include "Engine/Render/RenderHelpers.hpp"

#include "Engine/Scene/Components/StorageComponents.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/SceneRenderer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
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

    const ImageBasedLighting::Samplers& iblSamplers = imageBasedLighting.GetSamplers();

    const TextureSampler irradianceMap{ environmentComponent.irradianceTexture.view, iblSamplers.irradiance };
    const TextureSampler reflectionMap{ environmentComponent.reflectionTexture.view, iblSamplers.reflection };
    const TextureSampler specularBRDF{ imageBasedLighting.GetSpecularBRDF().view, iblSamplers.specularBRDF };

    descriptorProvider.PushGlobalData("irradianceMap", irradianceMap);
    descriptorProvider.PushGlobalData("reflectionMap", reflectionMap);
    descriptorProvider.PushGlobalData("specularBRDF", specularBRDF);
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
    if constexpr (Config::kRayTracingEnabled)
    {
        const auto& geometryComponent = scene.ctx().get<GeometryStorageComponent>();
        const auto& rayTracingComponent = scene.ctx().get<RayTracingSceneComponent>();

        std::vector<vk::Buffer> indexBuffers;
        std::vector<vk::Buffer> texCoordBuffers;

        indexBuffers.reserve(geometryComponent.primitives.size());
        texCoordBuffers.reserve(geometryComponent.primitives.size());

        for (const auto& primitive : geometryComponent.primitives)
        {
            indexBuffers.push_back(primitive.GetIndexBuffer());
            texCoordBuffers.push_back(primitive.GetTexCoordBuffer());
        }

        descriptorProvider.PushGlobalData("tlas", &rayTracingComponent.tlas);
        descriptorProvider.PushGlobalData("indexBuffers", &indexBuffers);
        descriptorProvider.PushGlobalData("texCoordBuffers", &texCoordBuffers);
    }
}

std::vector<MaterialPipeline> RenderHelpers::CreateMaterialPipelines(
        const Scene& scene, const RenderPass& renderPass,
        const CreateMaterialPipelinePred& createPipelinePred,
        const MaterialPipelineCreator& pipelineCreator)
{
    std::vector<MaterialPipeline> pipelines;

    const auto& materialComponent = scene.ctx().get<MaterialStorageComponent>();

    for (const auto& material : materialComponent.materials)
    {
        if (createPipelinePred(material.flags))
        {
            const auto pred = [&material](const MaterialPipeline& materialPipeline)
                {
                    return materialPipeline.materialFlags == material.flags;
                };

            const auto it = std::ranges::find_if(pipelines, pred);

            if (it == pipelines.end())
            {
                pipelines.emplace_back(material.flags, pipelineCreator(
                        renderPass, scene, material.flags));
            }
        }
    }

    return pipelines;
}

bool RenderHelpers::CheckPipelinesCompatibility(const std::vector<MaterialPipeline>& pipelines)
{
    const auto layoutsPred = [](const MaterialPipeline& a, const MaterialPipeline& b)
        {
            return a.pipeline->GetDescriptorSetLayouts() == b.pipeline->GetDescriptorSetLayouts();
        };

    return !pipelines.empty() && std::ranges::all_of(pipelines, [&](const MaterialPipeline& pipeline)
        {
            return layoutsPred(pipeline, pipelines.front());
        });
}
