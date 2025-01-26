#include "Engine/Render/Stages/ForwardStage.hpp"

#include "Engine/Engine.hpp"
#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/SceneRenderer.hpp"
#include "Engine/Render/Stages/GBufferStage.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/Pipelines/MaterialPipelineCache.hpp"
#include "Engine/Scene/Components/Components.hpp"

namespace Details
{
    static std::unique_ptr<RenderPass> CreateRenderPass()
    {
        const std::vector<RenderPass::AttachmentDescription> attachments{
            RenderPass::AttachmentDescription{
                RenderPass::AttachmentUsage::eColor,
                GBufferFormats::kSceneColor,
                vk::AttachmentLoadOp::eLoad,
                vk::AttachmentStoreOp::eStore,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eGeneral
            },
            RenderPass::AttachmentDescription{
                RenderPass::AttachmentUsage::eDepth,
                GBufferFormats::kDepthStencil,
                vk::AttachmentLoadOp::eLoad,
                vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eDepthStencilAttachmentOptimal,
                vk::ImageLayout::eDepthStencilAttachmentOptimal
            }
        };

        const RenderPass::Description description{
            vk::PipelineBindPoint::eGraphics,
            vk::SampleCountFlagBits::e1,
            attachments
        };

        const std::vector<PipelineBarrier> previousDependencies{
            PipelineBarrier{
                SyncScope::kComputeShaderWrite,
                SyncScope::kColorAttachmentRead
            },
            PipelineBarrier{
                SyncScope::kDepthStencilAttachmentWrite,
                SyncScope::kDepthStencilAttachmentRead
            },
        };

        const PipelineBarrier followingDependency{
            SyncScope::kColorAttachmentWrite,
            SyncScope::kColorAttachmentRead
        };

        std::unique_ptr<RenderPass> renderPass = RenderPass::Create(description,
                RenderPass::Dependencies{ previousDependencies, { followingDependency } });

        return renderPass;
    }

    static bool ShouldRenderMaterial(MaterialFlags flags)
    {
        if (RenderOptions::forceForward)
        {
            return true;
        }

        return !!(flags & MaterialFlagBits::eAlphaBlend);
    }

    static vk::Framebuffer CreateFramebuffer(const RenderPass& renderPass, const GBufferAttachments& gBuffer)
    {
        const vk::Device device = VulkanContext::device->Get();

        const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

        const std::vector<vk::ImageView> views{ gBuffer.sceneColor.view, gBuffer.depthStencil.view };

        return VulkanHelpers::CreateFramebuffers(device, renderPass.Get(), extent, {}, views).front();
    }

    static std::unique_ptr<MaterialPipelineCache> CreateMaterialPipelineCache(const RenderPass& renderPass)
    {
        return std::make_unique<MaterialPipelineCache>(MaterialPipelineStage::eForward, renderPass.Get());
    }

    static void CreateDescriptors(DescriptorProvider& descriptorProvider,
            const Scene& scene, const SceneRenderContext& context)
    {
        const auto& textureComponent = scene.ctx().get<TextureStorageComponent>();

        descriptorProvider.PushGlobalData("lights", context.uniforms.lights);
        descriptorProvider.PushGlobalData("materials", context.uniforms.materials);
        descriptorProvider.PushGlobalData("materialTextures", &textureComponent.textures);

        RenderHelpers::PushEnvironmentDescriptorData(descriptorProvider, scene);
        RenderHelpers::PushLightVolumeDescriptorData(descriptorProvider, scene);

        if (RenderOptions::rayTracingAllowed)
        {
            RenderHelpers::PushRayTracingDescriptorData(descriptorProvider, scene, context.tlas);
        }

        for (const auto& frameBuffer : context.uniforms.frames)
        {
            descriptorProvider.PushSliceData("frame", frameBuffer);
        }

        descriptorProvider.FlushData();
    }

    static std::vector<vk::ClearValue> GetClearValues()
    {
        return { VulkanHelpers::kDefaultClearColorValue, VulkanHelpers::GetDefaultClearDepthStencilValue() };
    }
}

ForwardStage::ForwardStage(const SceneRenderContext& context_)
    : RenderStage(context_)
{
    renderPass = Details::CreateRenderPass();

    framebuffer = Details::CreateFramebuffer(*renderPass, context.gBuffer);

    pipelineCache = Details::CreateMaterialPipelineCache(*renderPass);
}

ForwardStage::~ForwardStage()
{
    ForwardStage::RemoveScene();

    if (framebuffer)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }
}

void ForwardStage::RegisterScene(const Scene* scene_)
{
    RenderStage::RegisterScene(scene_);

    uniquePipelines = RenderHelpers::CacheMaterialPipelines(
            *scene, *pipelineCache, &Details::ShouldRenderMaterial);

    if (!uniquePipelines.empty())
    {
        Details::CreateDescriptors(pipelineCache->GetDescriptorProvider(), *scene, context);
    }
}

void ForwardStage::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    uniquePipelines.clear();

    scene = nullptr;
}

void ForwardStage::Update()
{
    Assert(scene);

    if (scene->ctx().get<MaterialStorageComponent>().updated)
    {
        uniquePipelines = RenderHelpers::CacheMaterialPipelines(
                *scene, *pipelineCache, &Details::ShouldRenderMaterial);
    }

    if (!uniquePipelines.empty())
    {
        const auto& textureComponent = scene->ctx().get<TextureStorageComponent>();
        const auto& geometryComponent = scene->ctx().get<GeometryStorageComponent>();

        DescriptorProvider& descriptorProvider = pipelineCache->GetDescriptorProvider();

        if (RenderOptions::rayTracingAllowed)
        {
            if (geometryComponent.updated || context.tlas.updated)
            {
                RenderHelpers::PushRayTracingDescriptorData(descriptorProvider, *scene, context.tlas);
            }
        }

        if (textureComponent.updated)
        {
            descriptorProvider.PushGlobalData("materialTextures", &textureComponent.textures);
        }

        descriptorProvider.FlushData();
    }
}

void ForwardStage::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Rect2D renderArea = VulkanHelpers::GetRect(context.GetRenderExtent());
    const vk::Viewport viewport = VulkanHelpers::GetViewport(context.GetRenderExtent());
    const std::vector<vk::ClearValue> clearValues = Details::GetClearValues();

    const vk::RenderPassBeginInfo beginInfo(
            renderPass->Get(), framebuffer,
            renderArea, clearValues);

    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

    commandBuffer.setViewport(0, { viewport });
    commandBuffer.setScissor(0, { renderArea });

    DrawScene(commandBuffer, imageIndex);

    commandBuffer.endRenderPass();
}

void ForwardStage::Resize()
{
    if (framebuffer)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }

    framebuffer = Details::CreateFramebuffer(*renderPass, context.gBuffer);
}

void ForwardStage::ReloadShaders()
{
    pipelineCache->ReloadPipelines();
}

void ForwardStage::DrawScene(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    Assert(scene);

    const auto sceneRenderView = scene->view<TransformComponent, RenderComponent>();

    const auto& materialComponent = scene->ctx().get<MaterialStorageComponent>();
    const auto& geometryComponent = scene->ctx().get<GeometryStorageComponent>();

    for (const auto& materialFlags : uniquePipelines)
    {
        const GraphicsPipeline& pipeline = pipelineCache->GetPipeline(materialFlags);

        const DescriptorProvider& descriptorProvider = pipelineCache->GetDescriptorProvider();

        pipeline.Bind(commandBuffer);

        pipeline.BindDescriptorSets(commandBuffer, descriptorProvider.GetDescriptorSlice(imageIndex));

        for (auto&& [entity, tc, rc] : sceneRenderView.each())
        {
            for (const auto& ro : rc.renderObjects)
            {
                if (materialComponent.materials[ro.material].flags == materialFlags)
                {
                    pipeline.PushConstant(commandBuffer, "transform", tc.GetWorldTransform().GetMatrix());

                    pipeline.PushConstant(commandBuffer, "materialIndex", ro.material);

                    const Primitive& primitive = geometryComponent.primitives[ro.primitive];

                    primitive.Draw(commandBuffer);
                }
            }
        }
    }
}
