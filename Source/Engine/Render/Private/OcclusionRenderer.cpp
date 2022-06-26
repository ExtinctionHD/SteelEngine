#include "Engine/Render/OcclusionRenderer.hpp"

#include "Engine/Render/Vulkan/PipelineHelpers.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Scene/SceneComponents.hpp"
#include "Engine/Scene/Components.hpp"
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

    static constexpr Camera::Description kCameraDescription{
        .type = Camera::Type::eOrthographic,
        .yFov = 0.0f,
        .width = 1.0f,
        .height = 1.0f,
        .zNear = 0.001f,
        .zFar = 1000.0f
    };

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
        constexpr BufferDescription bufferDescription{
            sizeof(glm::mat4),
            vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        return VulkanContext::bufferManager->CreateBuffer(
                bufferDescription, BufferCreateFlagBits::eStagingBuffer);
    }

    static DescriptorSet CreateCameraDescriptorSet(vk::Buffer cameraBuffer)
    {
        constexpr DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eVertex,
            vk::DescriptorBindingFlags()
        };

        return DescriptorHelpers::CreateDescriptorSet(
                { descriptorDescription }, { DescriptorHelpers::GetData(cameraBuffer) });
    }

    static std::unique_ptr<GraphicsPipeline> CreatePipeline(const RenderPass& renderPass,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
    {
        const ShaderDefines defines{ { "DEPTH_ONLY", 1 } };

        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eVertex,
                    Filepath("~/Shaders/Hybrid/GBuffer.vert"),
                    defines),
        };

        const uint32_t stride = PipelineHelpers::CalculateVertexSize(Primitive::Vertex::kFormat);

        const VertexDescription vertexDescription{
            { vk::Format::eR32G32B32Sfloat },
            stride, vk::VertexInputRate::eVertex
        };

        const std::vector<vk::PushConstantRange> pushConstantRanges{
            vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4)),
        };

        const GraphicsPipeline::Description description{
            vk::PrimitiveTopology::eTriangleList,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eNone,
            vk::FrontFace::eCounterClockwise,
            vk::SampleCountFlagBits::e1,
            vk::CompareOp::eLess,
            shaderModules,
            { vertexDescription },
            {},
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

    camera = std::make_unique<Camera>(Details::kCameraDescription);

    cameraData.buffer = Details::CreateCameraBuffer();
    cameraData.descriptorSet = Details::CreateCameraDescriptorSet(cameraData.buffer);

    pipeline = Details::CreatePipeline(*renderPass, { cameraData.descriptorSet.layout });
}

OcclusionRenderer::~OcclusionRenderer()
{
    DescriptorHelpers::DestroyDescriptorSet(cameraData.descriptorSet);
    VulkanContext::bufferManager->DestroyBuffer(cameraData.buffer);

    VulkanContext::device->Get().destroyQueryPool(queryPool);
    VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    VulkanContext::textureManager->DestroyTexture(depthTexture);
}

bool OcclusionRenderer::ContainsGeometry(const AABBox& bbox) const
{
    for (int32_t i = 0; i < 3; ++i)
    {
        PlaceCamera(bbox, i);

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                const glm::mat4 viewProj = camera->GetProjectionMatrix() * camera->GetViewMatrix();

                BufferHelpers::UpdateBuffer(commandBuffer, cameraData.buffer,
                        ByteView(viewProj), SyncScope::kWaitForNone, SyncScope::kVertexUniformRead);

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

void OcclusionRenderer::PlaceCamera(const AABBox& bbox, int32_t directionAxis) const
{
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

    camera->SetPosition(position);
    camera->SetDirection(direction);
    camera->SetUp(up);

    camera->SetZFar(zFar);
    camera->SetWidth(width);
    camera->SetHeight(height);

    camera->UpdateViewMatrix();
    camera->UpdateProjectionMatrix();
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

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->Get());

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            pipeline->GetLayout(), 0, { cameraData.descriptorSet.value }, {});

    const auto sceneView = scene->view<TransformComponent, RenderComponent>();

    const auto& geometryComponent = scene->ctx().at<SceneGeometryComponent>();

    for (auto&& [entity, tc, rc] : sceneView.each())
    {
        for (const auto& ro : rc.renderObjects)
        {
            const Primitive& primitive = geometryComponent.primitives[ro.primitive];

            commandBuffer.bindIndexBuffer(primitive.indexBuffer, 0, primitive.indexType);
            commandBuffer.bindVertexBuffers(0, { primitive.vertexBuffer }, { 0 });

            commandBuffer.pushConstants<glm::mat4>(pipeline->GetLayout(),
                vk::ShaderStageFlagBits::eVertex, 0, { tc.worldTransform });

            commandBuffer.drawIndexed(primitive.indexCount, 1, 0, 0, 0);
        }
    }

    commandBuffer.endRenderPass();
}
