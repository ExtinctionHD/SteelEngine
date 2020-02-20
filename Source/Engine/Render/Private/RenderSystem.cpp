#include "Engine/Render/RenderSystem.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCompiler.hpp"

#include "Utils/Helpers.hpp"
#include "Utils/Assert.hpp"

namespace SRenderSystem
{
    std::unique_ptr<RenderPass> CreateRenderPass(const VulkanContext &vulkanContext,
            bool hasUIRenderFunction)
    {
        AttachmentDescription attachmentDescription{
            AttachmentDescription::eUsage::kColor,
            vulkanContext.swapchain->GetFormat(),
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR
        };

        if (hasUIRenderFunction)
        {
            attachmentDescription.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
        }

        const RenderPassDescription description{
            vk::PipelineBindPoint::eGraphics, vk::SampleCountFlagBits::e1, { attachmentDescription }
        };

        std::unique_ptr<RenderPass> renderPass = RenderPass::Create(vulkanContext.device, description, {});

        return renderPass;
    }

    std::unique_ptr<GraphicsPipeline> CreateGraphicsPipeline(const VulkanContext &vulkanContext,
            const RenderPass &renderPass)
    {
        ShaderCompiler::Initialize();

        ShaderCache &shaderCache = *vulkanContext.shaderCache;
        const std::vector<ShaderModule> shaderModules{
            shaderCache.CreateShaderModule(vk::ShaderStageFlagBits::eVertex, Filepath("~/Shaders/Test.vert"), {}),
            shaderCache.CreateShaderModule(vk::ShaderStageFlagBits::eFragment, Filepath("~/Shaders/Test.frag"), {})
        };

        ShaderCompiler::Finalize();

        const VertexDescription vertexDescription{
            { vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat },
            vk::VertexInputRate::eVertex
        };

        const GraphicsPipelineDescription description{
            vulkanContext.swapchain->GetExtent(), vk::PrimitiveTopology::eTriangleList,
            vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise,
            vk::SampleCountFlagBits::e1, std::nullopt, shaderModules, { vertexDescription },
            { eBlendMode::kDisabled }, {}, {}
        };

        return GraphicsPipeline::Create(vulkanContext.device, renderPass.Get(), description);
    }

    RenderObject CreateRenderObject(const VulkanContext &vulkanContext)
    {
        struct Vertex
        {
            glm::vec3 position;
            glm::vec3 color;
        };

        const std::vector<Vertex> vertices{
            { glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },
            { glm::vec3(0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
            { glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
            { glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f) }
        };

        const std::vector<uint32_t> indices{
            0, 1, 2, 0, 2, 3
        };

        const BufferDescription vertexBufferDescription{
            sizeof(Vertex) * vertices.size(),
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const BufferDescription indexBufferDescription{
            sizeof(uint32_t) * indices.size(),
            vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const Mesh mesh{
            static_cast<uint32_t>(vertices.size()),
            VertexFormat{ vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat },
            vulkanContext.bufferManager->CreateBuffer(vertexBufferDescription, vertices),
            static_cast<uint32_t>(indices.size()), vk::IndexType::eUint32,
            vulkanContext.bufferManager->CreateBuffer(indexBufferDescription, indices)
        };

        const RenderObject renderObject{
            mesh, vulkanContext.blasGenerator->GenerateBlas(mesh)
        };

        return renderObject;
    }
}

RenderSystem::RenderSystem(std::shared_ptr<VulkanContext> aVulkanContext, const RenderFunction &aUIRenderFunction)
    : vulkanContext(aVulkanContext)
    , uiRenderFunction(aUIRenderFunction)
{
    renderPass = SRenderSystem::CreateRenderPass(GetRef(vulkanContext), static_cast<bool>(uiRenderFunction));
    pipeline = SRenderSystem::CreateGraphicsPipeline(GetRef(vulkanContext), GetRef(renderPass));
    renderObject = SRenderSystem::CreateRenderObject(GetRef(vulkanContext));

    framebuffers = VulkanHelpers::CreateSwapchainFramebuffers(GetRef(vulkanContext->device),
            GetRef(vulkanContext->swapchain), GetRef(renderPass));

    frames.resize(framebuffers.size());
    for (auto &frame : frames)
    {
        frame.commandBuffer = vulkanContext->device->AllocateCommandBuffer(eCommandsType::kOneTime);
        frame.presentCompleteSemaphore = VulkanHelpers::CreateSemaphore(GetRef(vulkanContext->device));
        frame.renderCompleteSemaphore = VulkanHelpers::CreateSemaphore(GetRef(vulkanContext->device));
        frame.fence = VulkanHelpers::CreateFence(GetRef(vulkanContext->device), vk::FenceCreateFlagBits::eSignaled);
    }

    drawingSuspended = false;
}

RenderSystem::~RenderSystem()
{
    for (auto &frame : frames)
    {
        vulkanContext->device->Get().destroySemaphore(frame.presentCompleteSemaphore);
        vulkanContext->device->Get().destroySemaphore(frame.renderCompleteSemaphore);
        vulkanContext->device->Get().destroyFence(frame.fence);
    }

    for (auto &framebuffer : framebuffers)
    {
        vulkanContext->device->Get().destroyFramebuffer(framebuffer);
    }
}

void RenderSystem::Process(float)
{
    vulkanContext->bufferManager->UpdateMarkedBuffers();
    vulkanContext->imageManager->UpdateMarkedImages();
    vulkanContext->resourceUpdateSystem->ExecuteUpdateCommands();

    DrawFrame();
}

void RenderSystem::OnResize(const vk::Extent2D &extent)
{
    drawingSuspended = extent.width == 0 || extent.height == 0;

    if (!drawingSuspended)
    {
        for (auto &framebuffer : framebuffers)
        {
            vulkanContext->device->Get().destroyFramebuffer(framebuffer);
        }

        renderPass = SRenderSystem::CreateRenderPass(GetRef(vulkanContext), static_cast<bool>(uiRenderFunction));
        pipeline = SRenderSystem::CreateGraphicsPipeline(GetRef(vulkanContext), GetRef(renderPass));
        framebuffers = VulkanHelpers::CreateSwapchainFramebuffers(GetRef(vulkanContext->device),
                GetRef(vulkanContext->swapchain), GetRef(renderPass));
    }
}

void RenderSystem::DrawFrame()
{
    if (drawingSuspended) return;

    const vk::SwapchainKHR swapchain = vulkanContext->swapchain->Get();
    const vk::Device device = vulkanContext->device->Get();

    auto [graphicsQueue, presentQueue] = vulkanContext->device->GetQueues();
    auto [commandBuffer, presentCompleteSemaphore, renderCompleteSemaphore, fence] = frames[frameIndex];

    auto [result, imageIndex] = vulkanContext->device->Get().acquireNextImageKHR(swapchain,
            std::numeric_limits<uint64_t>::max(), presentCompleteSemaphore,
            nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR) return;
    Assert(result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR);

    while (device.waitForFences(1, &fence, true, std::numeric_limits<uint64_t>::max()) == vk::Result::eTimeout) {}

    result = device.resetFences(1, &fence);
    Assert(result == vk::Result::eSuccess);

    const vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    result = commandBuffer.begin(beginInfo);
    Assert(result == vk::Result::eSuccess);

    Render(commandBuffer, imageIndex);

    if (uiRenderFunction)
    {
        uiRenderFunction(commandBuffer, imageIndex);
    }

    result = commandBuffer.end();
    Assert(result == vk::Result::eSuccess);

    const vk::PipelineStageFlags waitStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const vk::SubmitInfo submitInfo(1, &presentCompleteSemaphore, &waitStageFlags,
            1, &commandBuffer, 1, &renderCompleteSemaphore);

    result = graphicsQueue.submit(1, &submitInfo, fence);
    Assert(result == vk::Result::eSuccess);

    const vk::PresentInfoKHR presentInfo(1, &renderCompleteSemaphore, 1, &swapchain, &imageIndex, nullptr);

    result = presentQueue.presentKHR(presentInfo);
    Assert(result == vk::Result::eSuccess);

    frameIndex = (frameIndex + 1) % frames.size();
}

void RenderSystem::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Image image = vulkanContext->swapchain->GetImages()[imageIndex];
    vulkanContext->resourceUpdateSystem->GetLayoutUpdateCommands(image, VulkanHelpers::kSubresourceRangeFlatColor,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal)(commandBuffer);

    const vk::Extent2D &extent = vulkanContext->swapchain->GetExtent();

    const vk::Rect2D renderArea(vk::Offset2D(0, 0), extent);

    const vk::ClearValue clearValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });

    const vk::RenderPassBeginInfo beginInfo(renderPass->Get(), framebuffers[imageIndex], renderArea, 1, &clearValue);

    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->Get());

    const Mesh mesh = renderObject.mesh;
    const vk::DeviceSize offset = 0;

    commandBuffer.bindVertexBuffers(0, 1, &mesh.vertexBuffer->buffer, &offset);
    commandBuffer.bindIndexBuffer(mesh.indexBuffer->buffer, 0, mesh.indexType);

    commandBuffer.drawIndexed(mesh.indexCount, 1, 0, 0, 0);

    commandBuffer.endRenderPass();
}
