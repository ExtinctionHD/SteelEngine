#include "Engine/Render/Stages/LightingStage.hpp"

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/SceneRenderer.hpp"
#include "Engine/Render/Stages/DeferredStage.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Scene.hpp"

namespace Details
{
    static std::unique_ptr<ComputePipeline> CreatePipeline()
    {
        const ShaderDefines shaderDefines{
            { "RAY_TRACING_ENABLED", RenderOptions::rayTracingAllowed },
            { "LIGHT_VOLUME_ENABLED", 0 },
        };

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(
                Filepath("~/Shaders/Hybrid/Lighting.comp"), shaderDefines);

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static void CreateDescriptors(DescriptorProvider& descriptorProvider,
            const Scene& scene, const SceneRenderContext& context)
    {
        const auto& textureComponent = scene.ctx().get<TextureStorageComponent>();

        const vk::Sampler texelSampler = TextureCache::GetSampler(DefaultSampler::eTexelClamp);

        const Texture depthTexture{ context.gBuffer.depthStencil, texelSampler };

        descriptorProvider.PushGlobalData("depthTexture", &depthTexture);
        descriptorProvider.PushGlobalData("normalTexture", context.gBuffer.normal.view);
        descriptorProvider.PushGlobalData("sceneColorTexture", context.gBuffer.sceneColor.view);
        descriptorProvider.PushGlobalData("baseColorOcclusionTexture", context.gBuffer.baseColorOcclusion.view);
        descriptorProvider.PushGlobalData("roughnessMetallicTexture", context.gBuffer.roughnessMetallic.view);

        descriptorProvider.PushGlobalData("lights", context.uniforms.lights);

        RenderHelpers::PushEnvironmentDescriptorData(descriptorProvider, scene);
        RenderHelpers::PushLightVolumeDescriptorData(descriptorProvider, scene);

        if (RenderOptions::rayTracingAllowed)
        {
            RenderHelpers::PushRayTracingDescriptorData(descriptorProvider, scene, context.tlas);

            descriptorProvider.PushGlobalData("materials", context.uniforms.materials);
            descriptorProvider.PushGlobalData("materialTextures", &textureComponent.textures);
        }

        for (vk::Buffer frameBuffer : context.uniforms.frames)
        {
            descriptorProvider.PushSliceData("frame", frameBuffer);
        }

        descriptorProvider.FlushData();
    }
}

LightingStage::LightingStage(const SceneRenderContext& context_)
    : RenderStage(context_)
{
    pipeline = Details::CreatePipeline();

    descriptorProvider = pipeline->CreateDescriptorProvider();
}

LightingStage::~LightingStage()
{
    LightingStage::RemoveScene();
}

void LightingStage::RegisterScene(const Scene* scene_)
{
    RenderStage::RegisterScene(scene_);

    Details::CreateDescriptors(*descriptorProvider, *scene, context);
}

void LightingStage::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    descriptorProvider->Clear();

    scene = nullptr;
}

void LightingStage::Update()
{
    Assert(scene);

    if (RenderOptions::rayTracingAllowed)
    {
        const auto& textureComponent = scene->ctx().get<TextureStorageComponent>();
        const auto& geometryComponent = scene->ctx().get<GeometryStorageComponent>();

        if (geometryComponent.updated || context.tlas.updated)
        {
            RenderHelpers::PushRayTracingDescriptorData(*descriptorProvider, *scene, context.tlas);
        }

        if (textureComponent.updated)
        {
            descriptorProvider->PushGlobalData("materialTextures", &textureComponent.textures);
        }

        descriptorProvider->FlushData();
    }
}

void LightingStage::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    pipeline->Bind(commandBuffer);

    pipeline->BindDescriptorSets(commandBuffer, descriptorProvider->GetDescriptorSlice(imageIndex));

    pipeline->PushConstant(commandBuffer, "lightCount", scene->GetLightCount());

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(context.GetRenderExtent());

    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
}

void LightingStage::Resize()
{
    if (scene)
    {
        Details::CreateDescriptors(*descriptorProvider, *scene, context);
    }
}

void LightingStage::ReloadShaders()
{
    pipeline = Details::CreatePipeline();

    descriptorProvider = pipeline->CreateDescriptorProvider();

    if (scene)
    {
        Details::CreateDescriptors(*descriptorProvider, *scene, context);
    }
}
