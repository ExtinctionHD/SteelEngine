#include "Engine/Render/RenderSystem.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCompiler.hpp"

#include "Utils/Helpers.hpp"
#include "Utils/Assert.hpp"

namespace SRenderSystem
{
    std::unique_ptr<RenderPass> CreateRenderPass(bool hasUIRenderFunction)
    {
        AttachmentDescription attachmentDescription{
            AttachmentDescription::Usage::eColor,
            VulkanContext::swapchain->GetFormat(),
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

        std::unique_ptr<RenderPass> renderPass = RenderPass::Create(description, RenderPassDependencies{});

        return renderPass;
    }

    std::unique_ptr<GraphicsPipeline> CreateGraphicsPipeline(const RenderPass &renderPass,
            const std::vector<vk::DescriptorSetLayout> &descriptorSetLayouts)
    {
        ShaderCache &shaderCache = GetRef(VulkanContext::shaderCache);
        const std::vector<ShaderModule> shaderModules{
            shaderCache.CreateShaderModule(vk::ShaderStageFlagBits::eVertex, Filepath("~/Shaders/Rasterize.vert"), {}),
            shaderCache.CreateShaderModule(vk::ShaderStageFlagBits::eFragment, Filepath("~/Shaders/Rasterize.frag"), {})
        };

        const VertexDescription vertexDescription{
            { vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat },
            vk::VertexInputRate::eVertex
        };

        const std::vector<vk::PushConstantRange> pushConstantRanges{
            { vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4) },
            { vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4), sizeof(glm::vec4) }
        };

        const GraphicsPipelineDescription description{
            VulkanContext::swapchain->GetExtent(), vk::PrimitiveTopology::eTriangleList,
            vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise,
            vk::SampleCountFlagBits::e1, std::nullopt, shaderModules, { vertexDescription },
            { BlendMode::eDisabled }, descriptorSetLayouts, pushConstantRanges
        };

        return GraphicsPipeline::Create(renderPass.Get(), description);
    }

    std::unique_ptr<RayTracingPipeline> CreateRayTracingPipeline(const VulkanContext &vulkanContext,
            const std::vector<vk::DescriptorSetLayout> &descriptorSetLayouts)
    {
        ShaderCache &shaderCache = *vulkanContext.shaderCache;
        const std::vector<ShaderModule> shaderModules{
            shaderCache.CreateShaderModule(
                    vk::ShaderStageFlagBits::eRaygenNV, Filepath("~/Shaders/RayTrace.rgen"), {}),
            shaderCache.CreateShaderModule(
                    vk::ShaderStageFlagBits::eMissNV, Filepath("~/Shaders/RayTrace.rmiss"), {}),
            shaderCache.CreateShaderModule(
                    vk::ShaderStageFlagBits::eClosestHitNV, Filepath("~/Shaders/RayTrace.rchit"), {})
        };

        const std::vector<RayTracingShaderGroup> shaderGroups{
            { vk::RayTracingShaderGroupTypeNV::eGeneral, 0, VK_SHADER_UNUSED_NV, VK_SHADER_UNUSED_NV },
            { vk::RayTracingShaderGroupTypeNV::eGeneral, 1, VK_SHADER_UNUSED_NV, VK_SHADER_UNUSED_NV },
            { vk::RayTracingShaderGroupTypeNV::eTrianglesHitGroup, VK_SHADER_UNUSED_NV, 2, VK_SHADER_UNUSED_NV },
        };

        const RayTracingPipelineDescription description{
            shaderModules, shaderGroups, descriptorSetLayouts, {}
        };

        return RayTracingPipeline::Create(description);
    }

    Texture CreateTexture()
    {
        const SamplerDescription samplerDescription{
            vk::Filter::eLinear, vk::Filter::eLinear,
            vk::SamplerMipmapMode::eLinear,
            vk::SamplerAddressMode::eRepeat,
            std::nullopt, 0.0f, 0.0f
        };

        const Filepath texturePath("~/Assets/Textures/logo-256x256-solid.png");

        return VulkanContext::textureCache->GetTexture(texturePath, samplerDescription);
    }

    vk::AccelerationStructureNV GenerateBlas(const VulkanContext &vulkanContext, const RenderObject &renderObject)
    {
        return vulkanContext.accelerationStructureManager->GenerateBlas(renderObject);
    }

    vk::AccelerationStructureNV GenerateTlas(const VulkanContext &vulkanContext,
            vk::AccelerationStructureNV blas, const glm::mat4 &transform)
    {
        const GeometryInstance geometryInstance{ blas, transform };

        return vulkanContext.accelerationStructureManager->GenerateTlas({ geometryInstance });
    }

    BufferHandle CreateRayTracingCameraBuffer(const VulkanContext &vulkanContext)
    {
        const BufferDescription description{
            sizeof(CameraData),
            vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        return vulkanContext.bufferManager->CreateBuffer(description, BufferCreateFlagBits::eStagingBuffer);
    }
}

RenderSystem::RenderSystem(std::shared_ptr<Camera> camera_, const RenderFunction &uiRenderFunction_)
    : camera(camera_)
    , uiRenderFunction(uiRenderFunction_)
{
    renderPass = SRenderSystem::CreateRenderPass(static_cast<bool>(uiRenderFunction));

    CreateRasterizationDescriptors();

    CreateRayTracingDescriptors();
    UpdateRayTracingDescriptors();

    ShaderCompiler::Initialize();

    graphicsPipeline = SRenderSystem::CreateGraphicsPipeline(GetRef(renderPass),
            { rasterizationDescriptors.layout });
    /*rayTracingPipeline = SRenderSystem::CreateRayTracingPipeline(GetRef(vulkanContext),
            { rayTracingDescriptors.layout });*/

    ShaderCompiler::Finalize();

    const Device &device = GetRef(VulkanContext::device);
    const Swapchain &swapchain = GetRef(VulkanContext::swapchain);

    framebuffers = VulkanHelpers::CreateSwapchainFramebuffers(device.Get(),
            renderPass->Get(), swapchain.GetImageViews(), swapchain.GetExtent());

    frames.resize(framebuffers.size());
    for (auto &frame : frames)
    {
        frame.commandBuffer = device.AllocateCommandBuffer(CommandBufferType::eOneTime);
        frame.renderSync.waitSemaphores.push_back(VulkanHelpers::CreateSemaphore(device.Get()));
        frame.renderSync.signalSemaphores.push_back(VulkanHelpers::CreateSemaphore(device.Get()));
        frame.renderSync.fence = VulkanHelpers::CreateFence(device.Get(), vk::FenceCreateFlagBits::eSignaled);
        switch (renderFlow)
        {
        case RenderFlow::eRasterization:
            frame.renderSync.waitStages.emplace_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
            break;
        case RenderFlow::eRayTracing:
            frame.renderSync.waitStages.emplace_back(vk::PipelineStageFlagBits::eRayTracingShaderNV);
            break;
        }
    }

    switch (renderFlow)
    {
    case RenderFlow::eRasterization:
        mainRenderFunction = MakeFunction(&RenderSystem::Rasterize, this);
        break;
    case RenderFlow::eRayTracing:
        mainRenderFunction = MakeFunction(&RenderSystem::RayTrace, this);
        break;
    }

    drawingSuspended = false;
}

RenderSystem::~RenderSystem()
{
    for (auto &frame : frames)
    {
        VulkanHelpers::DestroyCommandBufferSync(VulkanContext::device->Get(), frame.renderSync);
    }

    for (auto &framebuffer : framebuffers)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }
}

void RenderSystem::Process(float)
{
    DrawFrame();
}

void RenderSystem::OnResize(const vk::Extent2D &extent)
{
    drawingSuspended = extent.width == 0 || extent.height == 0;

    if (!drawingSuspended)
    {
        for (auto &framebuffer : framebuffers)
        {
            VulkanContext::device->Get().destroyFramebuffer(framebuffer);
        }

        renderPass = SRenderSystem::CreateRenderPass(static_cast<bool>(uiRenderFunction));
        graphicsPipeline = SRenderSystem::CreateGraphicsPipeline(GetRef(renderPass),
                { rasterizationDescriptors.layout });
        framebuffers = VulkanHelpers::CreateSwapchainFramebuffers(VulkanContext::device->Get(), renderPass->Get(),
                VulkanContext::swapchain->GetImageViews(), VulkanContext::swapchain->GetExtent());

        UpdateRayTracingDescriptors();
    }
}

void RenderSystem::UpdateRayTracingResources(vk::CommandBuffer commandBuffer) const
{
    VulkanContext::bufferManager->UpdateBuffer(commandBuffer,
            rayTracingCameraBuffer, GetByteView(camera->GetData()));
}

void RenderSystem::DrawFrame()
{
    if (drawingSuspended) return;

    const vk::SwapchainKHR swapchain = VulkanContext::swapchain->Get();
    const vk::Device device = VulkanContext::device->Get();

    const auto &[graphicsQueue, presentQueue] = VulkanContext::device->GetQueues();
    const auto &[commandBuffer, renderingSync] = frames[frameIndex];

    const vk::Semaphore presentCompleteSemaphore = renderingSync.waitSemaphores.front();
    const vk::Semaphore renderingCompleteSemaphore = renderingSync.signalSemaphores.front();
    const vk::Fence renderingFence = renderingSync.fence;

    const auto acquireResult = VulkanContext::device->Get().acquireNextImageKHR(swapchain,
            Numbers::kMaxUint, presentCompleteSemaphore, nullptr);

    if (acquireResult.result == vk::Result::eErrorOutOfDateKHR) return;
    Assert(acquireResult.result == vk::Result::eSuccess || acquireResult.result == vk::Result::eSuboptimalKHR);

    VulkanHelpers::WaitForFences(VulkanContext::device->Get(), { renderingFence });

    const vk::Result resetResult = device.resetFences(1, &renderingFence);
    Assert(resetResult == vk::Result::eSuccess);

    const DeviceCommands renderingCommands = std::bind(&RenderSystem::Render,
            this, std::placeholders::_1, acquireResult.value);

    VulkanHelpers::SubmitCommandBuffer(graphicsQueue, commandBuffer,
            renderingCommands, renderingSync);

    const vk::PresentInfoKHR presentInfo(1, &renderingCompleteSemaphore,
            1, &swapchain, &acquireResult.value, nullptr);

    const vk::Result presentResult = presentQueue.presentKHR(presentInfo);
    Assert(presentResult == vk::Result::eSuccess);

    frameIndex = (frameIndex + 1) % frames.size();
}

void RenderSystem::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    mainRenderFunction(commandBuffer, imageIndex);
    if (uiRenderFunction)
    {
        uiRenderFunction(commandBuffer, imageIndex);
    }
}

void RenderSystem::Rasterize(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const ImageLayoutTransition swapchainImageLayoutTransition{
        vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
        PipelineBarrier{
            SyncScope{
                frames.front().renderSync.waitStages.front(),
                vk::AccessFlags()
            },
            SyncScope{
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::AccessFlagBits::eColorAttachmentWrite
            }
        }
    };

    VulkanHelpers::TransitImageLayout(commandBuffer, VulkanContext::swapchain->GetImages()[imageIndex],
            VulkanHelpers::kSubresourceRangeFlatColor, swapchainImageLayoutTransition);

    const vk::Extent2D &extent = VulkanContext::swapchain->GetExtent();

    const vk::Rect2D renderArea(vk::Offset2D(0, 0), extent);

    const vk::ClearValue clearValue(std::array<float, 4>{ 0.7f, 0.8f, 0.9f, 1.0f });

    const vk::RenderPassBeginInfo beginInfo(renderPass->Get(), framebuffers[imageIndex], renderArea, 1, &clearValue);

    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline->Get());
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphicsPipeline->GetLayout(),
            0, 1, &rasterizationDescriptors.descriptorSet, 0, nullptr);

    const glm::mat4 viewProjMatrix = camera->GetProjectionMatrix() * camera->GetViewMatrix();
    const glm::vec4 colorMultiplier(0.4f, 0.1f, 0.8f, 1.0f);
    commandBuffer.pushConstants(graphicsPipeline->GetLayout(), vk::ShaderStageFlagBits::eVertex,
            0, sizeof(glm::mat4), &viewProjMatrix);
    commandBuffer.pushConstants(graphicsPipeline->GetLayout(), vk::ShaderStageFlagBits::eFragment,
            sizeof(glm::mat4), sizeof(glm::vec4), &colorMultiplier);


    commandBuffer.endRenderPass();
}

void RenderSystem::RayTrace(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    UpdateRayTracingResources(commandBuffer);

    const ImageLayoutTransition preRayTraceLayoutTransition{
        vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
        PipelineBarrier{
            SyncScope{
                frames.front().renderSync.waitStages.front(),
                vk::AccessFlags()
            },
            SyncScope{
                vk::PipelineStageFlagBits::eRayTracingShaderNV,
                vk::AccessFlagBits::eShaderWrite
            }
        }
    };

    VulkanHelpers::TransitImageLayout(commandBuffer, VulkanContext::swapchain->GetImages()[imageIndex],
            VulkanHelpers::kSubresourceRangeFlatColor, preRayTraceLayoutTransition);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingNV, rayTracingPipeline->Get());

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingNV, rayTracingPipeline->GetLayout(),
            0, 1, &rayTracingDescriptors.descriptorSets[imageIndex], 0, nullptr);

    const ShaderBindingTable &shaderBindingTable = rayTracingPipeline->GetShaderBindingTable();
    const auto &[buffer, raygenOffset, missOffset, hitOffset, stride] = shaderBindingTable;

    const vk::Extent2D &extent = VulkanContext::swapchain->GetExtent();

    commandBuffer.traceRaysNV(buffer->buffer, raygenOffset,
            buffer->buffer, missOffset, stride,
            buffer->buffer, hitOffset, stride,
            nullptr, 0, 0, extent.width, extent.height, 1);

    const ImageLayoutTransition postRayTraceLayoutTransition{
        vk::ImageLayout::eGeneral, vk::ImageLayout::eColorAttachmentOptimal,
        PipelineBarrier{
            SyncScope{
                vk::PipelineStageFlagBits::eRayTracingShaderNV,
                vk::AccessFlagBits::eShaderWrite,
            },
            SyncScope{
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::AccessFlagBits::eColorAttachmentWrite
            },
        }
    };

    VulkanHelpers::TransitImageLayout(commandBuffer, VulkanContext::swapchain->GetImages()[imageIndex],
            VulkanHelpers::kSubresourceRangeFlatColor, postRayTraceLayoutTransition);
}

void RenderSystem::CreateRasterizationDescriptors()
{
    texture = SRenderSystem::CreateTexture();

    const DescriptorSetDescription description{
        { vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment },
    };

    auto &[layout, descriptorSet] = rasterizationDescriptors;

    layout = VulkanContext::descriptorPool->CreateDescriptorSetLayout(description);
    descriptorSet = VulkanContext::descriptorPool->AllocateDescriptorSet(layout);

    const vk::DescriptorImageInfo textureInfo{
        texture.sampler,
        texture.image->views.front(),
        vk::ImageLayout::eShaderReadOnlyOptimal
    };

    const DescriptorSetData descriptorSetData{
        { vk::DescriptorType::eCombinedImageSampler, textureInfo }
    };

    VulkanContext::descriptorPool->UpdateDescriptorSet(descriptorSet, descriptorSetData, 0);
}

void RenderSystem::CreateRayTracingDescriptors()
{
    /*tlas = SRenderSystem::GenerateTlas(GetRef(vulkanContext), blas, Matrix4::kIdentity);
    rayTracingCameraBuffer = SRenderSystem::CreateRayTracingCameraBuffer(GetRef(vulkanContext));

    const uint32_t imageCount = static_cast<uint32_t>(vulkanContext->swapchain->GetImageViews().size());

    const DescriptorSetDescription description{
        { vk::DescriptorType::eAccelerationStructureNV, vk::ShaderStageFlagBits::eRaygenNV },
        { vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eRaygenNV },
        { vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eRaygenNV }
    };

    auto &[layout, descriptorSets] = rayTracingDescriptors;

    layout = vulkanContext->descriptorPool->CreateDescriptorSetLayout(description);
    descriptorSets = vulkanContext->descriptorPool->AllocateDescriptorSets(layout, imageCount);*/
}

void RenderSystem::UpdateRayTracingDescriptors() const
{
    /*const std::vector<vk::ImageView> &swapchainImageViews = vulkanContext->swapchain->GetImageViews();
    const uint32_t swapchainImageCount = static_cast<uint32_t>(swapchainImageViews.size());

    const std::vector<vk::DescriptorSet> &descriptorSets = rayTracingDescriptors.descriptorSets;

    const DescriptorInfo tlasInfo = vk::WriteDescriptorSetAccelerationStructureNV(1, &tlas);
    const DescriptorInfo cameraBufferInfo = vk::DescriptorBufferInfo(
            rayTracingCameraBuffer->buffer, 0,
            rayTracingCameraBuffer->description.size);

    for (uint32_t i = 0; i < swapchainImageCount; ++i)
    {
        const DescriptorInfo swapchainImageInfo = vk::DescriptorImageInfo(vk::Sampler(),
                swapchainImageViews[i], vk::ImageLayout::eGeneral);

        const DescriptorSetData descriptorSetData{
            { vk::DescriptorType::eAccelerationStructureNV, tlasInfo },
            { vk::DescriptorType::eStorageImage, swapchainImageInfo },
            { vk::DescriptorType::eUniformBuffer, cameraBufferInfo }
        };

        vulkanContext->descriptorPool->UpdateDescriptorSet(descriptorSets[i], descriptorSetData, 0);
    }*/
}
