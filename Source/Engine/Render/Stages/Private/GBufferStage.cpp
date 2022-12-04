#include "Engine/Render/Stages/GBufferStage.hpp"

#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
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

        constexpr vk::ShaderStageFlags shaderStages = vk::ShaderStageFlagBits::eVertex;

        return RenderHelpers::CreateCameraData(bufferCount, bufferSize, shaderStages);
    }

    static DescriptorSet CreateMaterialDescriptorSet(const Scene& scene)
    {
        const auto& textureComponent = scene.ctx().get<TextureStorageComponent>();
        const auto& renderComponent = scene.ctx().get<RenderStorageComponent>();

        const uint32_t textureCount = static_cast<uint32_t>(textureComponent.textures.size());

        const DescriptorSetDescription descriptorSetDescription{
            DescriptorDescription{
                textureCount,
                vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eFragment,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                1, vk::DescriptorType::eUniformBuffer,
                vk::ShaderStageFlagBits::eFragment,
                vk::DescriptorBindingFlags()
            }
        };


        const DescriptorSetData descriptorSetData{
            DescriptorHelpers::GetData(textureComponent.textures),
            DescriptorHelpers::GetData(renderComponent.materialBuffer)
        };

        return DescriptorHelpers::CreateDescriptorSet(descriptorSetDescription, descriptorSetData);
    }

    static std::unique_ptr<GraphicsPipeline> CreatePipeline(const RenderPass& renderPass,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts,
            const MaterialFlags& materialFlags)
    {
        const ShaderDefines defines = MaterialHelpers::BuildShaderDefines(materialFlags);

        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eVertex,
                    Filepath("~/Shaders/Hybrid/GBuffer.vert"), defines),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eFragment,
                    Filepath("~/Shaders/Hybrid/GBuffer.frag"), defines)
        };

        const vk::CullModeFlagBits cullMode = materialFlags & MaterialFlagBits::eAlphaTest
                ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack;

        const VertexDescription vertexDescription{
            Primitive::Vertex::kFormat,
            0, vk::VertexInputRate::eVertex
        };

        const std::vector<BlendMode> blendModes = Repeat(BlendMode::eDisabled, GBufferStage::kFormats.size() - 1);

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
            { vertexDescription },
            blendModes,
            descriptorSetLayouts,
            pushConstantRanges
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
        std::vector<vk::ClearValue> clearValues(GBufferStage::kFormats.size());

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

    DescriptorHelpers::DestroyMultiDescriptorSet(cameraData.descriptorSet);

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

    materialDescriptorSet = Details::CreateMaterialDescriptorSet(*scene);

    materialPipelines = CreateMaterialPipelines(*scene, *renderPass, GetDescriptorSetLayouts());
}

void GBufferStage::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    for (auto& [materialFlags, pipeline] : materialPipelines)
    {
        pipeline.reset();
    }

    DescriptorHelpers::DestroyDescriptorSet(materialDescriptorSet);

    scene = nullptr;
}

void GBufferStage::Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const auto& cameraComponent = scene->ctx().get<CameraComponent>();

    const glm::mat4 viewProj = cameraComponent.projMatrix * cameraComponent.viewMatrix;

    BufferHelpers::UpdateBuffer(commandBuffer, cameraData.buffers[imageIndex],
            ByteView(viewProj), SyncScope::kWaitForNone, SyncScope::kVertexUniformRead);

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
    materialPipelines = CreateMaterialPipelines(*scene, *renderPass, GetDescriptorSetLayouts());
}

std::vector<GBufferStage::MaterialPipeline> GBufferStage::CreateMaterialPipelines(
        const Scene& scene, const RenderPass& renderPass,
        const std::vector<vk::DescriptorSetLayout>& layouts)
{
    std::vector<GBufferStage::MaterialPipeline> pipelines;

    const auto& materialComponent = scene.ctx().get<MaterialStorageComponent>();

    for (const auto& material : materialComponent.materials)
    {
        const auto pred = [&material](const MaterialPipeline& materialPipeline)
            {
                return materialPipeline.materialFlags == material.flags;
            };

        const auto it = std::ranges::find_if(pipelines, pred);

        if (it == pipelines.end())
        {
            std::unique_ptr<GraphicsPipeline> pipeline
                    = Details::CreatePipeline(renderPass, layouts, material.flags);

            pipelines.emplace_back(material.flags, std::move(pipeline));
        }
    }

    return pipelines;
}

std::vector<vk::DescriptorSetLayout> GBufferStage::GetDescriptorSetLayouts() const
{
    return { cameraData.descriptorSet.layout, materialDescriptorSet.layout };
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
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->Get());

        commandBuffer.pushConstants<glm::vec3>(pipeline->GetLayout(),
                vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4), { cameraPosition });

        const std::vector<vk::DescriptorSet> descriptorSets{
            cameraData.descriptorSet.values[imageIndex],
            materialDescriptorSet.value
        };

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                pipeline->GetLayout(), 0, descriptorSets, {});

        for (auto&& [entity, tc, rc] : sceneRenderView.each())
        {
            for (const auto& ro : rc.renderObjects)
            {
                if (materialComponent.materials[ro.material].flags == materialFlags)
                {
                    const Primitive& primitive = geometryComponent.primitives[ro.primitive];

                    commandBuffer.bindIndexBuffer(primitive.indexBuffer, 0, primitive.indexType);
                    commandBuffer.bindVertexBuffers(0, { primitive.vertexBuffer }, { 0 });

                    commandBuffer.pushConstants<glm::mat4>(pipeline->GetLayout(),
                            vk::ShaderStageFlagBits::eVertex, 0, { tc.worldTransform.GetMatrix() });

                    commandBuffer.pushConstants<uint32_t>(pipeline->GetLayout(),
                            vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4) + sizeof(glm::vec3), { ro.material });

                    commandBuffer.drawIndexed(primitive.indexCount, 1, 0, 0, 0);
                }
            }
        }
    }
}
