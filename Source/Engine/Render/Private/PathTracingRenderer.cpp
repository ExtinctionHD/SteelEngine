#include "Engine/Render/PathTracingRenderer.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"
#include "Engine/Render/Vulkan/Pipelines/RayTracingPipeline.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Components/EnvironmentComponent.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Engine.hpp"

namespace Details
{
    static constexpr uint32_t kSampleCount = 1;

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
        const ShaderDefines rayGenDefines{
            std::make_pair("ACCUMULATION", 1),
            std::make_pair("RENDER_TO_HDR", 0),
            std::make_pair("RENDER_TO_CUBE", 0),
            std::make_pair("SAMPLE_COUNT", kSampleCount),
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

    static void CreateDescriptors(DescriptorProvider& descriptorProvider,
            const Scene& scene, const RenderTarget& accumulationTarget)
    {
        const auto& renderComponent = scene.ctx().get<RenderContextComponent>();
        const auto& rayTracingComponent = scene.ctx().get<RayTracingContextComponent>();
        const auto& textureComponent = scene.ctx().get<TextureStorageComponent>();
        const auto& geometryComponent = scene.ctx().get<GeometryStorageComponent>();
        const auto& environmentComponent = scene.ctx().get<EnvironmentComponent>();

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

        descriptorProvider.PushGlobalData("lights", renderComponent.lightBuffer);
        descriptorProvider.PushGlobalData("materials", renderComponent.materialBuffer);
        descriptorProvider.PushGlobalData("materialTextures", &textureComponent.textures);
        descriptorProvider.PushGlobalData("environmentMap", &environmentComponent.cubemapTexture);
        descriptorProvider.PushGlobalData("tlas", &rayTracingComponent.tlas);
        descriptorProvider.PushGlobalData("indexBuffers", &indexBuffers);
        descriptorProvider.PushGlobalData("normalBuffers", &normalsBuffers);
        descriptorProvider.PushGlobalData("tangentBuffers", &tangentsBuffers);
        descriptorProvider.PushGlobalData("texCoordBuffers", &texCoordBuffers);
        descriptorProvider.PushGlobalData("accumulationTarget", accumulationTarget.view);

        for (uint32_t i = 0; i < VulkanContext::swapchain->GetImageCount(); ++i)
        {
            descriptorProvider.PushSliceData("frame", renderComponent.frameBuffers[i]);
            descriptorProvider.PushSliceData("renderTarget", VulkanContext::swapchain->GetImageViews()[i]);
        }

        descriptorProvider.FlushData();
    }
}

PathTracingRenderer::PathTracingRenderer()
{
    EASY_FUNCTION()

    accumulationTarget = Details::CreateAccumulationTarget(VulkanContext::swapchain->GetExtent());

    rayTracingPipeline = Details::CreateRayTracingPipeline();

    descriptorProvider = rayTracingPipeline->CreateDescriptorProvider();

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &PathTracingRenderer::HandleKeyInputEvent));

    Engine::AddEventHandler(EventType::eCameraUpdate,
            MakeFunction(this, &PathTracingRenderer::ResetAccumulation));
}

PathTracingRenderer::~PathTracingRenderer()
{
    RemoveScene();

    ResourceContext::DestroyResource(accumulationTarget);
}

void PathTracingRenderer::RegisterScene(const Scene* scene_)
{
    EASY_FUNCTION()

    RemoveScene();

    ResetAccumulation();

    scene = scene_;
    Assert(scene);

    Details::CreateDescriptors(*descriptorProvider, *scene, accumulationTarget);
}

void PathTracingRenderer::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    descriptorProvider->Clear();

    scene = nullptr;
}

void PathTracingRenderer::Update() const
{
    const auto& textureComponent = scene->ctx().get<TextureStorageComponent>();
    const auto& geometryComponent = scene->ctx().get<GeometryStorageComponent>();
    const auto& rayTracingComponent = scene->ctx().get<RayTracingContextComponent>();

    if (geometryComponent.updated)
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

        descriptorProvider->PushGlobalData("indexBuffers", &indexBuffers);
        descriptorProvider->PushGlobalData("normalBuffers", &normalsBuffers);
        descriptorProvider->PushGlobalData("tangentBuffers", &tangentsBuffers);
        descriptorProvider->PushGlobalData("texCoordBuffers", &texCoordBuffers);
    }

    if (rayTracingComponent.updated)
    {
        descriptorProvider->PushGlobalData("tlas", &rayTracingComponent.tlas);
    }

    if (textureComponent.updated)
    {
        descriptorProvider->PushGlobalData("materialTextures", &textureComponent.textures);
    }

    if (geometryComponent.updated || textureComponent.updated || rayTracingComponent.updated)
    {
        descriptorProvider->FlushData();
    }
}

void PathTracingRenderer::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    {
        const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];

        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral,
            PipelineBarrier{
                SyncScope::kWaitForNone,
                SyncScope::kRayTracingShaderWrite
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, swapchainImage,
                ImageHelpers::kFlatColor, layoutTransition);
    }

    if (scene)
    {
        rayTracingPipeline->Bind(commandBuffer);

        rayTracingPipeline->BindDescriptorSets(commandBuffer, descriptorProvider->GetDescriptorSlice(imageIndex));

        rayTracingPipeline->PushConstant(commandBuffer, "accumulationIndex", accumulationIndex++);

        const uint32_t lightCount = static_cast<uint32_t>(scene->view<LightComponent>().size());

        rayTracingPipeline->PushConstant(commandBuffer, "lightCount", lightCount);

        const vk::Extent2D extent = VulkanContext::swapchain->GetExtent();

        rayTracingPipeline->TraceRays(commandBuffer, VulkanHelpers::GetExtent3D(extent));
    }

    {
        const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];

        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eColorAttachmentOptimal,
            PipelineBarrier{
                SyncScope::kRayTracingShaderWrite,
                SyncScope::kColorAttachmentRead
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, swapchainImage,
                ImageHelpers::kFlatColor, layoutTransition);
    }
}

void PathTracingRenderer::Resize(const vk::Extent2D& extent)
{
    Assert(extent.width != 0 && extent.height != 0);

    ResetAccumulation();

    ResourceContext::DestroyResourceSafe(accumulationTarget);

    accumulationTarget = Details::CreateAccumulationTarget(VulkanContext::swapchain->GetExtent());

    if (scene)
    {
        Details::CreateDescriptors(*descriptorProvider, *scene, accumulationTarget);
    }
}

void PathTracingRenderer::HandleKeyInputEvent(const KeyInput& keyInput)
{
    if (keyInput.action == KeyAction::ePress)
    {
        switch (keyInput.key)
        {
        case Key::eR:
            ReloadShaders();
            break;
        default:
            break;
        }
    }
}

void PathTracingRenderer::ReloadShaders()
{
    if (!scene)
    {
        return;
    }

    VulkanContext::device->WaitIdle();

    ResetAccumulation();

    rayTracingPipeline = Details::CreateRayTracingPipeline();

    descriptorProvider = rayTracingPipeline->CreateDescriptorProvider();

    Details::CreateDescriptors(*descriptorProvider, *scene, accumulationTarget);
}

void PathTracingRenderer::ResetAccumulation()
{
    accumulationIndex = 0;
}
