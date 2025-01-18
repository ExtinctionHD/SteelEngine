#include "Engine/Render/Stages/LightingStage.hpp"

#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/SceneRenderer.hpp"
#include "Engine/Render/Stages/GBufferStage.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Scene.hpp"

namespace Details
{
    static constexpr glm::uvec3 kWorkGroupSize(8, 8, 1);

    static std::unique_ptr<ComputePipeline> CreatePipeline()
    {
        const ShaderDefines shaderDefines{
            { "RAY_TRACING_ENABLED", RenderOptions::rayTracingAllowed },
            { "LIGHT_VOLUME_ENABLED", 0 },
        };

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(
                Filepath("~/Shaders/Hybrid/Lighting.comp"), kWorkGroupSize, shaderDefines);

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static void CreateDescriptors(DescriptorProvider& descriptorProvider,
            const Scene& scene, const GBufferAttachments& gBuffer)
    {
        const auto& renderComponent = scene.ctx().get<RenderContextComponent>();
        const auto& textureComponent = scene.ctx().get<TextureStorageComponent>();

        const vk::Sampler texelSampler = TextureCache::GetSampler(DefaultSampler::eTexelClamp);

        const Texture depthTexture{ gBuffer.depthStencil, texelSampler };

        descriptorProvider.PushGlobalData("lights", renderComponent.uniforms.lights);

        descriptorProvider.PushGlobalData("depthTexture", &depthTexture);
        descriptorProvider.PushGlobalData("normalTexture", gBuffer.normal.view);
        descriptorProvider.PushGlobalData("sceneColorTexture", gBuffer.sceneColor.view);
        descriptorProvider.PushGlobalData("baseColorOcclusionTexture", gBuffer.baseColorOcclusion.view);
        descriptorProvider.PushGlobalData("roughnessMetallicTexture", gBuffer.roughnessMetallic.view);

        RenderHelpers::PushEnvironmentDescriptorData(scene, descriptorProvider);
        RenderHelpers::PushLightVolumeDescriptorData(scene, descriptorProvider);

        if (RenderOptions::rayTracingAllowed)
        {
            RenderHelpers::PushRayTracingDescriptorData(scene, descriptorProvider);

            descriptorProvider.PushGlobalData("materials", renderComponent.uniforms.materials);
            descriptorProvider.PushGlobalData("materialTextures", &textureComponent.textures);
        }

        for (vk::Buffer frameBuffer : renderComponent.uniforms.frames)
        {
            descriptorProvider.PushSliceData("frame", frameBuffer);
        }

        descriptorProvider.FlushData();
    }
}

LightingStage::LightingStage()
{
    pipeline = Details::CreatePipeline();

    descriptorProvider = pipeline->CreateDescriptorProvider();
}

LightingStage::~LightingStage()
{
    RemoveScene();
}

void LightingStage::RegisterScene(const Scene* scene_)
{
    RemoveScene();

    scene = scene_;
    Assert(scene);

    const auto& renderComponent = scene->ctx().get<RenderContextComponent>();
    Details::CreateDescriptors(*descriptorProvider, *scene, renderComponent.gBuffer);
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

void LightingStage::Update() const
{
    Assert(scene);

    if (RenderOptions::rayTracingAllowed)
    {
        const auto& textureComponent = scene->ctx().get<TextureStorageComponent>();
        const auto& geometryComponent = scene->ctx().get<GeometryStorageComponent>();
        const auto& renderComponent = scene->ctx().get<RenderContextComponent>();

        if (geometryComponent.updated || renderComponent.tlas.updated)
        {
            RenderHelpers::PushRayTracingDescriptorData(*scene, *descriptorProvider);
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
    // TODO rename to renderContext / sceneRenderContext / sceneRenderComponent
    const auto& renderComponent = scene->ctx().get<RenderContextComponent>();

    const vk::Extent2D extent = renderComponent.gBuffer.GetExtent();

    pipeline->Bind(commandBuffer);

    pipeline->BindDescriptorSets(commandBuffer, descriptorProvider->GetDescriptorSlice(imageIndex));

    pipeline->PushConstant(commandBuffer, "lightCount", scene->GetLightCount());

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent, Details::kWorkGroupSize);

    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
}

void LightingStage::Resize() const
{
    if (scene)
    {
        const auto& renderContext = scene->ctx().get<RenderContextComponent>(); // TODO create GetContext method
        Details::CreateDescriptors(*descriptorProvider, *scene, renderContext.gBuffer);
    }
}

void LightingStage::ReloadShaders()
{
    Assert(scene);

    pipeline = Details::CreatePipeline();

    descriptorProvider = pipeline->CreateDescriptorProvider();

    const auto& renderComponent = scene->ctx().get<RenderContextComponent>();
    Details::CreateDescriptors(*descriptorProvider, *scene, renderComponent.gBuffer);
}
