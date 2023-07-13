#include "Engine/Render/OcclusionRenderer.hpp"

#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Components/StorageComponents.hpp"
#include "Engine/Scene/Components/TransformComponent.hpp"
#include "Engine/Scene/Primitive.hpp"
#include "Engine/Scene/Scene.hpp"

#include "Utils/AABBox.hpp"

namespace Details
{
    static constexpr vk::Format kDepthFormat = vk::Format::eD32Sfloat;

    static constexpr vk::Extent2D kExtent(1024, 1024);

    static constexpr vk::Rect2D kRenderArea(vk::Offset2D(), kExtent);

    static constexpr vk::Viewport kViewport(
            0.0f, 0.0f,
            static_cast<float>(kExtent.width),
            static_cast<float>(kExtent.height),
            0.0f, 1.0f);

    static constexpr float zNear = 0.001f;

    static Texture CreateDepthTexture()
    {
        const Texture texture = ImageHelpers::CreateRenderTarget(kDepthFormat, kExtent,
                vk::SampleCountFlagBits::e1, vk::ImageUsageFlagBits::eDepthStencilAttachment);

        VulkanContext::device->ExecuteOneTimeCommands([&texture](vk::CommandBuffer commandBuffer)
            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    PipelineBarrier::kEmpty
                };

                ImageHelpers::TransitImageLayout(commandBuffer, texture.image,
                        ImageHelpers::kFlatDepth, layoutTransition);
            });

        return texture;
    }

    static std::unique_ptr<RenderPass> CreateRenderPass()
    {
        const RenderPass::AttachmentDescription attachment{
            RenderPass::AttachmentUsage::eDepth,
            kDepthFormat,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        };

        const RenderPass::Description description{
            vk::PipelineBindPoint::eGraphics,
            vk::SampleCountFlagBits::e1,
            { attachment }
        };

        std::unique_ptr<RenderPass> renderPass
                = RenderPass::Create(description, RenderPass::Dependencies{});

        return renderPass;
    }

    static vk::Framebuffer CreateFramebuffer(
            const RenderPass& renderPass, vk::ImageView imageView)
    {
        const vk::Device device = VulkanContext::device->Get();

        return VulkanHelpers::CreateFramebuffers(device,
                renderPass.Get(), kExtent, {}, { imageView }).front();
    }

    static vk::QueryPool CreateQueryPool()
    {
        const vk::QueryPoolCreateInfo createInfo{
            {}, vk::QueryType::eOcclusion, 1
        };

        const auto [result, queryPool] = VulkanContext::device->Get().createQueryPool(createInfo);
        Assert(result == vk::Result::eSuccess);

        return queryPool;
    }

    static vk::Buffer CreateCameraBuffer()
    {
        return BufferHelpers::CreateEmptyBuffer(
                vk::BufferUsageFlagBits::eUniformBuffer, sizeof(glm::mat4));
    }

    static std::unique_ptr<GraphicsPipeline> CreatePipeline(const RenderPass& renderPass)
    {
        const ShaderDefines defines{ { "DEPTH_ONLY", 1 } };

        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    Filepath("~/Shaders/Hybrid/GBuffer.vert"),
                    vk::ShaderStageFlagBits::eVertex,
                    defines),
        };

        const VertexInput vertexInput{
            { vk::Format::eR32G32B32Sfloat },
            0, vk::VertexInputRate::eVertex
        };

        const GraphicsPipeline::Description description{
            vk::PrimitiveTopology::eTriangleList,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eNone,
            vk::FrontFace::eCounterClockwise,
            vk::SampleCountFlagBits::e1,
            vk::CompareOp::eLess,
            shaderModules,
            { vertexInput },
            {}
        };

        std::unique_ptr<GraphicsPipeline> pipeline = GraphicsPipeline::Create(renderPass.Get(), description);

        for (const auto& shaderModule : shaderModules)
        {
            VulkanContext::shaderManager->DestroyShaderModule(shaderModule);
        }

        return pipeline;
    }

    static glm::mat4 ComputeViewProj(const AABBox& bbox, int32_t directionAxis)
    {
        Assert(directionAxis >= 0);

        const int32_t rightAxis = (directionAxis + 1) % 3;
        const int32_t upAxis = (directionAxis + 2) % 3;

        glm::vec3 direction = Vector3::kZero;
        direction[directionAxis] = -1.0f;

        glm::vec3 right = Vector3::kZero;
        right[rightAxis] = 1.0f;

        glm::vec3 up = Vector3::kZero;
        up[upAxis] = 1.0f;

        const glm::vec3 size = bbox.GetSize();

        const float zFar = size[directionAxis];
        const float width = size[rightAxis];
        const float height = size[upAxis];

        const glm::vec3 offset = direction * zFar * 0.5f;
        const glm::vec3 position = bbox.GetCenter() - offset;

        const CameraLocation location{
            position, direction, up
        };

        const CameraProjection projection{
            0.0f, width, height, zNear, zFar
        };

        const glm::mat4 view = CameraHelpers::ComputeViewMatrix(location);
        const glm::mat4 proj = CameraHelpers::ComputeProjMatrix(projection);

        return proj * view;
    }

    static uint64_t GetQueryPoolResult(vk::QueryPool queryPool)
    {
        const vk::Device device = VulkanContext::device->Get();

        const auto [result, sampleCount] = device.getQueryPoolResult<uint64_t>(
                queryPool, 0, 1, sizeof(uint64_t), vk::QueryResultFlagBits::e64);

        Assert(result == vk::Result::eSuccess);

        return sampleCount;
    }
}

OcclusionRenderer::OcclusionRenderer(const Scene* scene_)
    : scene(scene_)
{
    depthTexture = Details::CreateDepthTexture();
    renderPass = Details::CreateRenderPass();
    framebuffer = Details::CreateFramebuffer(*renderPass, depthTexture.view);

    queryPool = Details::CreateQueryPool();

    cameraBuffer = Details::CreateCameraBuffer();

    pipeline = Details::CreatePipeline(*renderPass);

    descriptorProvider = pipeline->CreateDescriptorProvider();
    descriptorProvider->PushGlobalData("viewProj", cameraBuffer);
    descriptorProvider->FlushData();
}

OcclusionRenderer::~OcclusionRenderer()
{
    VulkanContext::bufferManager->DestroyBuffer(cameraBuffer);

    VulkanContext::device->Get().destroyQueryPool(queryPool);
    VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    VulkanContext::textureManager->DestroyTexture(depthTexture);
}

bool OcclusionRenderer::ContainsGeometry(const AABBox& bbox) const
{
    for (int32_t i = 0; i < 3; ++i)
    {
        const glm::mat4 viewProj = Details::ComputeViewProj(bbox, i);

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                BufferHelpers::UpdateBuffer(commandBuffer, cameraBuffer,
                        GetByteView(viewProj), SyncScope::kWaitForNone, SyncScope::kVertexUniformRead);

                commandBuffer.resetQueryPool(queryPool, 0, 1);

                commandBuffer.beginQuery(queryPool, 0, vk::QueryControlFlags());

                Render(commandBuffer);

                commandBuffer.endQuery(queryPool, 0);
            });

        const uint64_t sampleCount = Details::GetQueryPoolResult(queryPool);

        if (sampleCount > 0)
        {
            return true;
        }
    }

    return false;
}

void OcclusionRenderer::Render(vk::CommandBuffer commandBuffer) const
{
    const vk::ClearValue clearValue = VulkanHelpers::kDefaultClearDepthStencilValue;

    const vk::RenderPassBeginInfo beginInfo(
            renderPass->Get(), framebuffer,
            Details::kRenderArea, clearValue);

    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

    commandBuffer.setViewport(0, { Details::kViewport });
    commandBuffer.setScissor(0, { Details::kRenderArea });

    pipeline->Bind(commandBuffer);

    pipeline->BindDescriptorSets(commandBuffer, descriptorProvider->GetDescriptorSlice());

    const auto sceneRenderView = scene->view<TransformComponent, RenderComponent>();

    const auto& geometryComponent = scene->ctx().get<GeometryStorageComponent>();

    for (auto&& [entity, tc, rc] : sceneRenderView.each())
    {
        for (const auto& ro : rc.renderObjects)
        {
            pipeline->PushConstant(commandBuffer, "transform", tc.GetWorldTransform().GetMatrix());

            const Primitive& primitive = geometryComponent.primitives[ro.primitive];

            commandBuffer.bindIndexBuffer(primitive.GetIndexBuffer(), 0, vk::IndexType::eUint32);
            commandBuffer.bindVertexBuffers(0, { primitive.GetPositionBuffer() }, { 0 });

            commandBuffer.drawIndexed(primitive.GetIndexCount(), 1, 0, 0, 0);
        }
    }

    commandBuffer.endRenderPass();
}
