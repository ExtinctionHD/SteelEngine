#include "Engine/Render/Stages/DeferredStage.hpp"

#include "Engine/Engine.hpp"
#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/SceneRenderer.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Pipelines/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/Pipelines/MaterialPipelineCache.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Primitive.hpp"
#include "Engine/Scene/Scene.hpp"

namespace Details
{
    static std::unique_ptr<RenderPass> CreateRenderPass()
    {
        std::vector<RenderPass::AttachmentDescription> attachments(GBufferAttachments::GetCount());

        for (size_t i = 0; i < GBufferAttachments::GetCount(); ++i)
        {
            if (ImageHelpers::IsDepthFormat(GBufferFormats::kFormats[i]))
            {
                attachments[i] = RenderPass::AttachmentDescription{
                    RenderPass::AttachmentUsage::eDepth,
                    GBufferFormats::kFormats[i],
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    vk::ImageLayout::eShaderReadOnlyOptimal
                };
            }
            else
            {
                attachments[i] = RenderPass::AttachmentDescription{
                    RenderPass::AttachmentUsage::eColor,
                    GBufferFormats::kFormats[i],
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    vk::ImageLayout::eGeneral,
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::ImageLayout::eGeneral
                };
            }
        }

        const RenderPass::Description description{
            vk::PipelineBindPoint::eGraphics,
            vk::SampleCountFlagBits::e1,
            attachments
        };

        // TODO synchronize sequential frames

        // TODO implement previous dependency

        const std::vector<PipelineBarrier> followingDependencies{
            PipelineBarrier{
                SyncScope::kColorAttachmentWrite | SyncScope::kDepthStencilAttachmentWrite,
                SyncScope::kComputeShaderRead
            }
        };

        std::unique_ptr<RenderPass> renderPass = RenderPass::Create(description,
                RenderPass::Dependencies{ {}, followingDependencies });

        return renderPass;
    }

    static bool ShouldRenderMaterial(MaterialFlags flags)
    {
        if (RenderOptions::forceForward)
        {
            return false;
        }

        return !(flags & MaterialFlagBits::eAlphaBlend);
    }

    static vk::Framebuffer CreateFramebuffer(const RenderPass& renderPass, const GBufferAttachments& gBuffer)
    {
        const vk::Device device = VulkanContext::device->Get();

        std::vector<vk::ImageView> views;
        views.reserve(GBufferAttachments::GetCount());

        for (const auto& [image, view] : gBuffer.GetArray())
        {
            views.push_back(view);
        }

        return VulkanHelpers::CreateFramebuffers(device,
                renderPass.Get(), gBuffer.GetExtent(), {}, views).front();
    }

    static std::unique_ptr<MaterialPipelineCache> CreatePipelineCache(const RenderPass& renderPass)
    {
        return std::make_unique<MaterialPipelineCache>(MaterialPipelineStage::eGBuffer, renderPass.Get());
    }

    static void CreateDescriptors(DescriptorProvider& descriptorProvider,
            const Scene& scene, const SceneRenderContext& context)
    {
        const auto& textureComponent = scene.ctx().get<TextureStorageComponent>();

        descriptorProvider.PushGlobalData("materials", context.uniforms.materials);
        descriptorProvider.PushGlobalData("materialTextures", &textureComponent.textures);

        for (const auto& frameBuffer : context.uniforms.frames)
        {
            descriptorProvider.PushSliceData("frame", frameBuffer);
        }

        descriptorProvider.FlushData();
    }

    static std::vector<vk::ClearValue> GetClearValues()
    {
        std::vector<vk::ClearValue> clearValues(GBufferAttachments::GetCount());

        for (size_t i = 0; i < clearValues.size(); ++i)
        {
            if (ImageHelpers::IsDepthFormat(GBufferFormats::kFormats[i]))
            {
                clearValues[i] = VulkanHelpers::GetDefaultClearDepthStencilValue();
            }
            else
            {
                clearValues[i] = VulkanHelpers::kDefaultClearColorValue;
            }
        }

        return clearValues;
    }
}

DeferredStage::DeferredStage(const SceneRenderContext& context_)
    : RenderStage(context_)
{
    renderPass = Details::CreateRenderPass();

    framebuffer = Details::CreateFramebuffer(*renderPass, context.gBuffer);

    pipelineCache = Details::CreatePipelineCache(*renderPass);
}

DeferredStage::~DeferredStage()
{
    DeferredStage::RemoveScene();

    if (framebuffer)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }
}

void DeferredStage::RegisterScene(const Scene* scene_)
{
    RenderStage::RegisterScene(scene_);

    uniquePipelines = RenderHelpers::CacheMaterialPipelines(
            *scene, *pipelineCache, &Details::ShouldRenderMaterial);

    if (!uniquePipelines.empty())
    {
        Details::CreateDescriptors(pipelineCache->GetDescriptorProvider(), *scene, context);
    }
}

void DeferredStage::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    uniquePipelines.clear();

    scene = nullptr;
}

void DeferredStage::Update()
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

        if (textureComponent.updated)
        {
            DescriptorProvider& descriptorProvider = pipelineCache->GetDescriptorProvider();

            descriptorProvider.PushGlobalData("materialTextures", &textureComponent.textures);

            descriptorProvider.FlushData();
        }
    }
}

void DeferredStage::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
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

void DeferredStage::Resize()
{
    if (framebuffer)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }

    framebuffer = Details::CreateFramebuffer(*renderPass, context.gBuffer);
}

void DeferredStage::ReloadShaders()
{
    pipelineCache->ReloadPipelines();
}

void DeferredStage::DrawScene(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    Assert(scene);

    const auto& materialComponent = scene->ctx().get<MaterialStorageComponent>();
    const auto& geometryComponent = scene->ctx().get<GeometryStorageComponent>();

    for (const auto& materialFlags : uniquePipelines)
    {
        const GraphicsPipeline& pipeline = pipelineCache->GetPipeline(materialFlags);

        const DescriptorProvider& descriptorProvider = pipelineCache->GetDescriptorProvider();

        pipeline.Bind(commandBuffer);

        pipeline.BindDescriptorSets(commandBuffer, descriptorProvider.GetDescriptorSlice(imageIndex));

        for (auto&& [entity, tc, rc] : scene->view<TransformComponent, RenderComponent>().each())
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
