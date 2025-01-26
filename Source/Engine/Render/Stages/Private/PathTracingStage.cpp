#include "Engine/Render/Stages/PathTracingStage.hpp"

#include "Engine/Engine.hpp"
#include "Engine/Render/SceneRenderer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"
#include "Engine/Render/Vulkan/Pipelines/RayTracingPipeline.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Components/EnvironmentComponent.hpp"
#include "Engine/Scene/Scene.hpp"

namespace Details
{
    static int32_t sampleCount = 1;
    static CVarInt sampleCountCVar("r.PathTracing.SampleCount", sampleCount); // TODO ReloadShaders in callback

    static RenderTarget CreateAccumulationTarget(const vk::Extent2D& extent)
    {
        const RenderTarget renderTarget = ResourceContext::CreateBaseImage({
            .format = vk::Format::eR32G32B32A32Sfloat,
            .extent = extent
        });

        VulkanContext::device->ExecuteOneTimeCommands([&renderTarget](vk::CommandBuffer commandBuffer)
            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eGeneral,
                    PipelineBarrier::kEmpty
                };

                ImageHelpers::TransitImageLayout(commandBuffer, renderTarget.image,
                        ImageHelpers::kFlatColor, layoutTransition);
            });

        return renderTarget;
    }

    static std::unique_ptr<RayTracingPipeline> CreateRayTracingPipeline()
    {
        // TODO remove unused flags
        const ShaderDefines rayGenDefines{
            std::make_pair("ACCUMULATION", 1),
            std::make_pair("RENDER_TO_HDR", 0),
            std::make_pair("RENDER_TO_CUBE", 0),
            std::make_pair("SAMPLE_COUNT", sampleCount),
        };

        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    Filepath("~/Shaders/PathTracing/RayGen.rgen"),
                    vk::ShaderStageFlagBits::eRaygenKHR,
                    rayGenDefines),
            VulkanContext::shaderManager->CreateShaderModule(
                    Filepath("~/Shaders/PathTracing/Miss.rmiss"),
                    vk::ShaderStageFlagBits::eMissKHR),
            VulkanContext::shaderManager->CreateShaderModule(
                    Filepath("~/Shaders/PathTracing/ClosestHit.rchit"),
                    vk::ShaderStageFlagBits::eClosestHitKHR),
            VulkanContext::shaderManager->CreateShaderModule(
                    Filepath("~/Shaders/PathTracing/AnyHit.rahit"),
                    vk::ShaderStageFlagBits::eAnyHitKHR)
        };

        ShaderGroupMap shaderGroupMap;

        shaderGroupMap[ShaderGroupType::eRaygen] = {
            ShaderGroup{ 0, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR }
        };
        shaderGroupMap[ShaderGroupType::eMiss] = {
            ShaderGroup{ 1, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR }
        };
        shaderGroupMap[ShaderGroupType::eHit] = {
            ShaderGroup{ VK_SHADER_UNUSED_KHR, 2, 3, VK_SHADER_UNUSED_KHR }
        };

        const RayTracingPipeline::Description description{ shaderModules, shaderGroupMap };

        std::unique_ptr<RayTracingPipeline> pipeline = RayTracingPipeline::Create(description);

        for (const auto& shaderModule : shaderModules)
        {
            VulkanContext::shaderManager->DestroyShaderModule(shaderModule);
        }

        return pipeline;
    }

    static void PushGeometryDescriptorData(DescriptorProvider& descriptorProvider,
            const GeometryStorageComponent& geometryComponent)
    {
        std::vector<vk::Buffer> indexBuffers;
        std::vector<vk::Buffer> normalsBuffers;
        std::vector<vk::Buffer> tangentsBuffers;
        std::vector<vk::Buffer> texCoordBuffers;

        indexBuffers.reserve(geometryComponent.primitives.size());
        normalsBuffers.reserve(geometryComponent.primitives.size());
        tangentsBuffers.reserve(geometryComponent.primitives.size());
        texCoordBuffers.reserve(geometryComponent.primitives.size());

        for (const auto& primitive : geometryComponent.primitives)
        {
            indexBuffers.push_back(primitive.GetIndexBuffer());
            normalsBuffers.push_back(primitive.GetNormalBuffer());
            tangentsBuffers.push_back(primitive.GetTangentBuffer());
            texCoordBuffers.push_back(primitive.GetTexCoordBuffer());
        }

        descriptorProvider.PushGlobalData("indexBuffers", &indexBuffers);
        descriptorProvider.PushGlobalData("normalBuffers", &normalsBuffers);
        descriptorProvider.PushGlobalData("tangentBuffers", &tangentsBuffers);
        descriptorProvider.PushGlobalData("texCoordBuffers", &texCoordBuffers);
    }

    static void CreateDescriptors(DescriptorProvider& descriptorProvider, const Scene& scene,
            const SceneRenderContext& context, const RenderTarget& accumulationTarget)
    {
        const auto& textureComponent = scene.ctx().get<TextureStorageComponent>();
        const auto& environmentComponent = scene.ctx().get<EnvironmentComponent>();
        const auto& geometryComponent = scene.ctx().get<GeometryStorageComponent>();

        PushGeometryDescriptorData(descriptorProvider, geometryComponent);

        descriptorProvider.PushGlobalData("lights", context.uniforms.lights);
        descriptorProvider.PushGlobalData("materials", context.uniforms.materials);
        descriptorProvider.PushGlobalData("materialTextures", &textureComponent.textures);
        descriptorProvider.PushGlobalData("environmentMap", &environmentComponent.cubemapTexture);
        descriptorProvider.PushGlobalData("tlas", &context.tlas);

        descriptorProvider.PushGlobalData("sceneColorTarget", context.gBuffer.sceneColor.view);
        descriptorProvider.PushGlobalData("accumulationTarget", accumulationTarget.view);

        for (const auto& frameBuffer : context.uniforms.frames)
        {
            descriptorProvider.PushSliceData("frame", frameBuffer);
        }

        descriptorProvider.FlushData();
    }
}

PathTracingStage::PathTracingStage(const SceneRenderContext& context_)
    : RenderStage(context_)
{
    rayTracingPipeline = Details::CreateRayTracingPipeline();

    descriptorProvider = rayTracingPipeline->CreateDescriptorProvider();

    accumulationTarget = Details::CreateAccumulationTarget(context.GetRenderExtent());

    Engine::AddEventHandler(EventType::eCameraUpdate,
            MakeFunction(this, &PathTracingStage::ResetAccumulation));
}

PathTracingStage::~PathTracingStage()
{
    PathTracingStage::RemoveScene();

    ResourceContext::DestroyResource(accumulationTarget);
}

void PathTracingStage::RegisterScene(const Scene* scene_)
{
    RenderStage::RegisterScene(scene_);

    Details::CreateDescriptors(*descriptorProvider, *scene, context, accumulationTarget);
}

void PathTracingStage::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    descriptorProvider->Clear();

    scene = nullptr;
}

void PathTracingStage::Update()
{
    const auto& textureComponent = scene->ctx().get<TextureStorageComponent>();
    const auto& geometryComponent = scene->ctx().get<GeometryStorageComponent>();

    if (geometryComponent.updated)
    {
        Details::PushGeometryDescriptorData(*descriptorProvider, geometryComponent);
    }

    if (textureComponent.updated)
    {
        descriptorProvider->PushGlobalData("materialTextures", &textureComponent.textures);
    }

    if (context.tlas.updated)
    {
        descriptorProvider->PushGlobalData("tlas", &context.tlas);
    }

    if (geometryComponent.updated || textureComponent.updated || context.tlas.updated)
    {
        descriptorProvider->FlushData();
    }
}

void PathTracingStage::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    Assert(scene);

    rayTracingPipeline->Bind(commandBuffer);

    rayTracingPipeline->BindDescriptorSets(commandBuffer, descriptorProvider->GetDescriptorSlice(imageIndex));

    rayTracingPipeline->PushConstant(commandBuffer, "accumulationIndex", accumulationIndex++);

    rayTracingPipeline->PushConstant(commandBuffer, "lightCount", scene->GetLightCount());

    rayTracingPipeline->TraceRays(commandBuffer, VulkanHelpers::GetExtent3D(context.GetRenderExtent()));
}

void PathTracingStage::Resize()
{
    ResetAccumulation();

    ResourceContext::DestroyResource(accumulationTarget);

    accumulationTarget = Details::CreateAccumulationTarget(context.GetRenderExtent());

    if (scene)
    {
        Details::CreateDescriptors(*descriptorProvider, *scene, context, accumulationTarget);
    }
}

void PathTracingStage::ReloadShaders()
{
    ResetAccumulation();

    rayTracingPipeline = Details::CreateRayTracingPipeline();

    descriptorProvider = rayTracingPipeline->CreateDescriptorProvider();

    Details::CreateDescriptors(*descriptorProvider, *scene, context, accumulationTarget);
}

void PathTracingStage::ResetAccumulation() const
{
    accumulationIndex = 0;
}
