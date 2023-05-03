#include "Engine/Render/Stages/GBufferStage.hpp"

#include "Engine/Render/Vulkan/Pipelines/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Scene/StorageComponents.hpp"
#include "Engine/Scene/Components.hpp"
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
                    vk::ImageLayout::eShaderReadOnlyOptimal
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
                    vk::ImageLayout::eGeneral
                };
            }
        }

        const RenderPass::Description description{
            vk::PipelineBindPoint::eGraphics,
            vk::SampleCountFlagBits::e1,
            attachments
        };

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

    static std::vector<Texture> CreateRenderTargets()
    {
        const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

        std::vector<Texture> renderTargets(GBufferStage::kFormats.size());

        for (size_t i = 0; i < renderTargets.size(); ++i)
        {
            constexpr vk::ImageUsageFlags colorImageUsage
                    = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage;

            constexpr vk::ImageUsageFlags depthImageUsage
                    = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;

            const vk::Format format = GBufferStage::kFormats[i];

            const vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;

            const vk::ImageUsageFlags imageUsage = ImageHelpers::IsDepthFormat(format)
                    ? depthImageUsage : colorImageUsage;

            renderTargets[i] = ImageHelpers::CreateRenderTarget(
                    format, extent, sampleCount, imageUsage);
        }

        VulkanContext::device->ExecuteOneTimeCommands([&renderTargets](vk::CommandBuffer commandBuffer)
            {
                const ImageLayoutTransition colorLayoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eGeneral,
                    PipelineBarrier::kEmpty
                };

                const ImageLayoutTransition depthLayoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    PipelineBarrier::kEmpty
                };

                for (size_t i = 0; i < renderTargets.size(); ++i)
                {
                    const vk::Image image = renderTargets[i].image;

                    const vk::Format format = GBufferStage::kFormats[i];

                    const vk::ImageSubresourceRange& subresourceRange = ImageHelpers::IsDepthFormat(format)
                            ? ImageHelpers::kFlatDepth : ImageHelpers::kFlatColor;

                    const ImageLayoutTransition& layoutTransition = ImageHelpers::IsDepthFormat(format)
                            ? depthLayoutTransition : colorLayoutTransition;

                    ImageHelpers::TransitImageLayout(commandBuffer, image, subresourceRange, layoutTransition);
                }
            });

        return renderTargets;
    }

    static vk::Framebuffer CreateFramebuffer(const RenderPass& renderPass,
            const std::vector<vk::ImageView>& imageViews)
    {
        const vk::Device device = VulkanContext::device->Get();

        const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

        return VulkanHelpers::CreateFramebuffers(device, renderPass.Get(), extent, {}, imageViews).front();
    }

    static CameraData CreateCameraData()
    {
        const uint32_t bufferCount = VulkanContext::swapchain->GetImageCount();

        constexpr vk::DeviceSize bufferSize = sizeof(glm::mat4);

        return RenderHelpers::CreateCameraData(bufferCount, bufferSize);
    }

    static bool CreateMaterialPipelinePred(MaterialFlags materialFlags)
    {
        if constexpr (Config::kForceForward)
        {
            return false;
        }
        else
        {
            return !(materialFlags & MaterialFlagBits::eAlphaBlend);
        }
    }

    static std::unique_ptr<GraphicsPipeline> CreateMaterialPipeline(const RenderPass& renderPass,
            const MaterialFlags& materialFlags, const Scene& scene)
    {
        const auto& materialComponent = scene.ctx().get<MaterialStorageComponent>();

        ShaderDefines defines = MaterialHelpers::BuildShaderDefines(materialFlags);

        defines.emplace("MATERIAL_COUNT", static_cast<uint32_t>(materialComponent.materials.size()));

        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    Filepath("~/Shaders/Hybrid/GBuffer.vert"),
                    vk::ShaderStageFlagBits::eVertex, defines),
            VulkanContext::shaderManager->CreateShaderModule(
                    Filepath("~/Shaders/Hybrid/GBuffer.frag"),
                    vk::ShaderStageFlagBits::eFragment, defines)
        };

        const vk::CullModeFlagBits cullMode = materialFlags & MaterialFlagBits::eDoubleSided
                ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack;

        const std::vector<BlendMode> blendModes(GBufferStage::kColorAttachmentCount, BlendMode::eDisabled);

        const std::vector<vk::PushConstantRange> pushConstantRanges{
            vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4)),
            vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4),
                    sizeof(glm::vec3) + sizeof(uint32_t))
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
            blendModes
        };

        std::unique_ptr<GraphicsPipeline> pipeline = GraphicsPipeline::Create(renderPass.Get(), description);

        for (const auto& shaderModule : shaderModules)
        {
            VulkanContext::shaderManager->DestroyShaderModule(shaderModule);
        }

        return pipeline;
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

    cameraData = Details::CreateCameraData();
}

GBufferStage::~GBufferStage()
{
    RemoveScene();

    for (const auto& buffer : cameraData.buffers)
    {
        VulkanContext::bufferManager->DestroyBuffer(buffer);
    }

    for (const auto& texture : renderTargets)
    {
        VulkanContext::textureManager->DestroyTexture(texture);
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

    materialPipelines = RenderHelpers::CreateMaterialPipelines(*scene, *renderPass,
            &Details::CreateMaterialPipelinePred, &Details::CreateMaterialPipeline);

    CreateDescriptorProvider();
}

void GBufferStage::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    descriptorProvider.FreeDescriptors();

    for (auto& [materialFlags, pipeline] : materialPipelines)
    {
        pipeline.reset();
    }

    scene = nullptr;
}

void GBufferStage::Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const auto& cameraComponent = scene->ctx().get<CameraComponent>();

    const glm::mat4 viewProj = cameraComponent.projMatrix * cameraComponent.viewMatrix;

    BufferHelpers::UpdateBuffer(commandBuffer, cameraData.buffers[imageIndex],
            GetByteView(viewProj), SyncScope::kWaitForNone, SyncScope::kVertexUniformRead);

    const vk::Rect2D renderArea = RenderHelpers::GetSwapchainRenderArea();
    const vk::Viewport viewport = RenderHelpers::GetSwapchainViewport();
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

void GBufferStage::Resize()
{
    for (const auto& texture : renderTargets)
    {
        VulkanContext::textureManager->DestroyTexture(texture);
    }

    VulkanContext::device->Get().destroyFramebuffer(framebuffer);

    renderTargets = Details::CreateRenderTargets();

    framebuffer = Details::CreateFramebuffer(*renderPass, GetImageViews());
}

void GBufferStage::ReloadShaders()
{
    materialPipelines = RenderHelpers::CreateMaterialPipelines(*scene, *renderPass,
            &Details::CreateMaterialPipelinePred, &Details::CreateMaterialPipeline);
}

void GBufferStage::CreateDescriptorProvider()
{
    if (materialPipelines.empty())
    {
        return;
    }

    descriptorProvider.Allocate(materialPipelines.front().pipeline->GetDescriptorSetLayouts());

    const auto& renderComponent = scene->ctx().get<RenderStorageComponent>();
    const auto& textureComponent = scene->ctx().get<TextureStorageComponent>();

    const DescriptorSetData globalDescriptorSetData{
        DescriptorHelpers::GetData(renderComponent.materialBuffer),
        DescriptorHelpers::GetData(textureComponent.textures),
    };

    descriptorProvider.UpdateGlobalDescriptorSet(globalDescriptorSetData);

    for (uint32_t i = 0; i < descriptorProvider.GetSliceCount(); ++i)
    {
        const DescriptorSetData frameDescriptorSetData{
            DescriptorHelpers::GetData(cameraData.buffers[i])
        };

        descriptorProvider.UpdateFrameDescriptorSet(i, frameDescriptorSetData);
    }
}

void GBufferStage::DrawScene(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const auto& cameraComponent = scene->ctx().get<CameraComponent>();

    const glm::vec3& cameraPosition = cameraComponent.location.position;

    const auto sceneRenderView = scene->view<TransformComponent, RenderComponent>();

    const auto& materialComponent = scene->ctx().get<MaterialStorageComponent>();
    const auto& geometryComponent = scene->ctx().get<GeometryStorageComponent>();

    for (const auto& [materialFlags, pipeline] : materialPipelines)
    {
        pipeline->Bind(commandBuffer);

        pipeline->BindDescriptorSets(commandBuffer, 0, descriptorProvider.GetDescriptorSlice(imageIndex));

        pipeline->PushConstant(commandBuffer, "cameraPosition", cameraPosition);

        for (auto&& [entity, tc, rc] : sceneRenderView.each())
        {
            for (const auto& ro : rc.renderObjects)
            {
                if (materialComponent.materials[ro.material].flags == materialFlags)
                {
                    pipeline->PushConstant(commandBuffer, "transform", tc.worldTransform.GetMatrix());

                    pipeline->PushConstant(commandBuffer, "materialIndex", ro.material);

                    const Primitive& primitive = geometryComponent.primitives[ro.primitive];

                    PrimitiveHelpers::DrawPrimitive(commandBuffer, primitive);
                }
            }
        }
    }
}
