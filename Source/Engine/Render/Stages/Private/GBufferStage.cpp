#include "Engine/Render/Stages/GBufferStage.hpp"

#include "Engine/Engine.hpp"
#include "Engine/Render/Vulkan/Pipelines/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Primitive.hpp"
#include "Engine/Scene/Scene.hpp"

namespace Details
{
    static std::unique_ptr<RenderPass> CreateRenderPass()
    {
        std::vector<RenderPass::AttachmentDescription> attachments(GBufferStage::kFormats.size());

        for (size_t i = 0; i < attachments.size(); ++i)
        {
            if (ImageHelpers::IsDepthFormat(GBufferStage::kFormats[i]))
            {
                attachments[i] = RenderPass::AttachmentDescription{
                    RenderPass::AttachmentUsage::eDepth,
                    GBufferStage::kFormats[i],
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                };
            }
            else
            {
                attachments[i] = RenderPass::AttachmentDescription{
                    RenderPass::AttachmentUsage::eColor,
                    GBufferStage::kFormats[i],
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    vk::ImageLayout::eGeneral,
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::ImageLayout::eGeneral,
                };
            }
        }

        const RenderPass::Description description{
            vk::PipelineBindPoint::eGraphics,
            vk::SampleCountFlagBits::e1,
            attachments,
        };

        const std::vector<PipelineBarrier> followingDependencies{
            PipelineBarrier{
                SyncScope::kColorAttachmentWrite | SyncScope::kDepthStencilAttachmentWrite,
                SyncScope::kComputeShaderRead,
            },
        };

        std::unique_ptr<RenderPass> renderPass =
                RenderPass::Create(description, RenderPass::Dependencies{ {}, followingDependencies });

        return renderPass;
    }

    static std::vector<Texture> CreateRenderTargets()
    {
        const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

        std::vector<Texture> renderTargets(GBufferStage::kFormats.size());

        for (size_t i = 0; i < renderTargets.size(); ++i)
        {
            constexpr vk::ImageUsageFlags colorImageUsage =
                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage;

            constexpr vk::ImageUsageFlags depthImageUsage =
                    vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;

            const vk::Format format = GBufferStage::kFormats[i];

            const vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;

            const vk::ImageUsageFlags imageUsage =
                    ImageHelpers::IsDepthFormat(format) ? depthImageUsage : colorImageUsage;

            renderTargets[i] = ImageHelpers::CreateRenderTarget(format, extent, sampleCount, imageUsage);
        }

        VulkanContext::device->ExecuteOneTimeCommands(
                [&renderTargets](vk::CommandBuffer commandBuffer)
                {
                    const ImageLayoutTransition colorLayoutTransition{ vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eGeneral, PipelineBarrier::kEmpty };

                    const ImageLayoutTransition depthLayoutTransition{ vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eDepthStencilAttachmentOptimal, PipelineBarrier::kEmpty };

                    for (size_t i = 0; i < renderTargets.size(); ++i)
                    {
                        const vk::Image image = renderTargets[i].image;

                        const vk::Format format = GBufferStage::kFormats[i];

                        const vk::ImageSubresourceRange& subresourceRange = ImageHelpers::IsDepthFormat(format)
                                ? ImageHelpers::kFlatDepth
                                : ImageHelpers::kFlatColor;

                        const ImageLayoutTransition& layoutTransition =
                                ImageHelpers::IsDepthFormat(format) ? depthLayoutTransition : colorLayoutTransition;

                        ImageHelpers::TransitImageLayout(commandBuffer, image, subresourceRange, layoutTransition);
                    }
                });

        return renderTargets;
    }

    static vk::Framebuffer CreateFramebuffer(const RenderPass& renderPass, const std::vector<vk::ImageView>& imageViews)
    {
        const vk::Device device = VulkanContext::device->Get();

        const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

        return VulkanHelpers::CreateFramebuffers(device, renderPass.Get(), extent, {}, imageViews).front();
    }

    static bool CreateMaterialPipelinePred(MaterialFlags materialFlags)
    {
        if (Config::engine.forceForward)
        {
            return false;
        }
        else
        {
            return !(materialFlags & MaterialFlagBits::eAlphaBlend);
        }
    }

    static std::unique_ptr<GraphicsPipeline> CreateMaterialPipeline(
            const RenderPass& renderPass, const Scene&, MaterialFlags materialFlags)
    {
        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    Filepath("~/Shaders/Hybrid/GBuffer.vert"), vk::ShaderStageFlagBits::eVertex),
            VulkanContext::shaderManager->CreateShaderModule(
                    Filepath("~/Shaders/Hybrid/GBuffer.frag"), vk::ShaderStageFlagBits::eFragment),
        };

        const vk::CullModeFlagBits cullMode = materialFlags & MaterialFlagBits::eDoubleSided
                ? vk::CullModeFlagBits::eNone
                : vk::CullModeFlagBits::eBack;

        const std::vector<BlendMode> blendModes(GBufferStage::kColorAttachmentCount, BlendMode::eDisabled);

        const std::vector<vk::PushConstantRange> pushConstantRanges{
            vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4)),
            vk::PushConstantRange(
                    vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4), sizeof(glm::vec3) + sizeof(uint32_t)),
        };

        const GraphicsPipeline::Description description{
            vk::PrimitiveTopology::eTriangleList,
            vk::PolygonMode::eFill,
            cullMode,
            vk::FrontFace::eCounterClockwise,
            vk::SampleCountFlagBits::e1,
            vk::CompareOp::eLess,
            shaderModules,
            Primitive::kVertexInputs,
            blendModes,
        };

        std::unique_ptr<GraphicsPipeline> pipeline = GraphicsPipeline::Create(renderPass.Get(), description);

        for (const auto& shaderModule : shaderModules)
        {
            VulkanContext::shaderManager->DestroyShaderModule(shaderModule);
        }

        return pipeline;
    }

    static void CreateDescriptors(DescriptorProvider& descriptorProvider, const Scene& scene)
    {
        const auto& renderComponent = scene.ctx().get<RenderContextComponent>();
        const auto& textureComponent = scene.ctx().get<TextureStorageComponent>();

        descriptorProvider.PushGlobalData("materials", renderComponent.materialBuffer);
        descriptorProvider.PushGlobalData("materialTextures", &textureComponent.textureSamplers);

        for (const auto& frameBuffer : renderComponent.frameBuffers)
        {
            descriptorProvider.PushSliceData("frame", frameBuffer);
        }

        descriptorProvider.FlushData();
    }

    static std::vector<vk::ClearValue> GetClearValues()
    {
        std::vector<vk::ClearValue> clearValues(GBufferStage::kAttachmentCount);

        for (size_t i = 0; i < clearValues.size(); ++i)
        {
            if (ImageHelpers::IsDepthFormat(GBufferStage::kFormats[i]))
            {
                clearValues[i] = VulkanHelpers::kDefaultClearDepthStencilValue;
            }
            else
            {
                clearValues[i] = VulkanHelpers::kDefaultClearColorValue;
            }
        }

        return clearValues;
    }
}

GBufferStage::GBufferStage()
{
    renderPass = Details::CreateRenderPass();

    renderTargets = Details::CreateRenderTargets();

    framebuffer = Details::CreateFramebuffer(*renderPass, GetImageViews());
}

GBufferStage::~GBufferStage()
{
    RemoveScene();

    for (const auto& texture : renderTargets)
    {
        ResourceHelpers::DestroyResource(texture);
    }

    VulkanContext::device->Get().destroyFramebuffer(framebuffer);
}

std::vector<vk::ImageView> GBufferStage::GetImageViews() const
{
    return TextureHelpers::GetViews(renderTargets);
}

vk::ImageView GBufferStage::GetDepthImageView() const
{
    return renderTargets.back().view;
}

void GBufferStage::RegisterScene(const Scene* scene_)
{
    RemoveScene();

    scene = scene_;

    materialPipelines = RenderHelpers::CreateMaterialPipelines(
            *scene, *renderPass, &Details::CreateMaterialPipelinePred, &Details::CreateMaterialPipeline);

    if (!materialPipelines.empty())
    {
        Assert(RenderHelpers::CheckPipelinesCompatibility(materialPipelines));

        descriptorProvider = materialPipelines.front().pipeline->CreateDescriptorProvider();

        Details::CreateDescriptors(*descriptorProvider, *scene);
    }
}

void GBufferStage::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    descriptorProvider.reset();

    for (auto& [materialFlags, pipeline] : materialPipelines)
    {
        pipeline.reset();
    }

    scene = nullptr;
}

void GBufferStage::Update()
{
    Assert(scene);

    if (scene->ctx().get<MaterialStorageComponent>().updated)
    {
        RenderHelpers::UpdateMaterialPipelines(materialPipelines, *scene, *renderPass,
                &Details::CreateMaterialPipelinePred, &Details::CreateMaterialPipeline);
    }

    if (!materialPipelines.empty())
    {
        Assert(RenderHelpers::CheckPipelinesCompatibility(materialPipelines));

        const auto textureComponent = scene->ctx().get<TextureStorageComponent>();

        if (textureComponent.updated)
        {
            descriptorProvider->PushGlobalData("materialTextures", &textureComponent.textureSamplers);

            descriptorProvider->FlushData();
        }
    }
}

void GBufferStage::Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Rect2D renderArea = RenderHelpers::GetSwapchainRenderArea();
    const vk::Viewport viewport = RenderHelpers::GetSwapchainViewport();
    const std::vector<vk::ClearValue> clearValues = Details::GetClearValues();

    const vk::RenderPassBeginInfo beginInfo(renderPass->Get(), framebuffer, renderArea, clearValues);

    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

    commandBuffer.setViewport(0, { viewport });
    commandBuffer.setScissor(0, { renderArea });

    DrawScene(commandBuffer, imageIndex);

    commandBuffer.endRenderPass();
}

void GBufferStage::Resize()
{
    for (const auto& texture : renderTargets)
    {
        ResourceHelpers::DestroyResource(texture);
    }

    VulkanContext::device->Get().destroyFramebuffer(framebuffer);

    renderTargets = Details::CreateRenderTargets();

    framebuffer = Details::CreateFramebuffer(*renderPass, GetImageViews());
}

void GBufferStage::ReloadShaders()
{
    Assert(scene);

    materialPipelines = RenderHelpers::CreateMaterialPipelines(
            *scene, *renderPass, &Details::CreateMaterialPipelinePred, &Details::CreateMaterialPipeline);

    if (!materialPipelines.empty())
    {
        descriptorProvider = materialPipelines.front().pipeline->CreateDescriptorProvider();

        Details::CreateDescriptors(*descriptorProvider, *scene);
    }
}

void GBufferStage::DrawScene(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    Assert(scene);

    const auto sceneRenderView = scene->view<TransformComponent, RenderComponent>();

    const auto& materialComponent = scene->ctx().get<MaterialStorageComponent>();
    const auto& geometryComponent = scene->ctx().get<GeometryStorageComponent>();

    for (const auto& [materialFlags, pipeline] : materialPipelines)
    {
        pipeline->Bind(commandBuffer);

        pipeline->BindDescriptorSets(commandBuffer, descriptorProvider->GetDescriptorSlice(imageIndex));

        for (auto&& [entity, tc, rc] : sceneRenderView.each())
        {
            for (const auto& ro : rc.renderObjects)
            {
                if (materialComponent.materials[ro.material].flags == materialFlags)
                {
                    pipeline->PushConstant(commandBuffer, "transform", tc.GetWorldTransform().GetMatrix());

                    pipeline->PushConstant(commandBuffer, "materialIndex", ro.material);

                    const Primitive& primitive = geometryComponent.primitives[ro.primitive];

                    primitive.Draw(commandBuffer);
                }
            }
        }
    }
}
