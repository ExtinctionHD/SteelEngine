#include "Engine/Render/RenderSystem.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"

#include "Utils/Helpers.hpp"
#include "Utils/Assert.hpp"

namespace SRenderSystem
{
    std::unique_ptr<RayTracingPipeline> CreateRayTracingPipeline(
            const std::vector<vk::DescriptorSetLayout> &descriptorSetLayouts)
    {
        ShaderCache &shaderCache = *VulkanContext::shaderCache;
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
            VulkanConfig::kMaxAnisotropy,
            0.0f, std::numeric_limits<float>::max()
        };

        const Filepath texturePath("~/Assets/Scenes/DamagedHelmet/Default_albedo.jpg");

        return VulkanContext::textureCache->GetTexture(texturePath, samplerDescription);
    }

    vk::AccelerationStructureNV GenerateBlas(const RenderObject &renderObject)
    {
        return VulkanContext::accelerationStructureManager->GenerateBlas(renderObject);
    }

    vk::AccelerationStructureNV GenerateTlas(vk::AccelerationStructureNV blas, const glm::mat4 &transform)
    {
        const GeometryInstance geometryInstance{ blas, transform };

        return VulkanContext::accelerationStructureManager->GenerateTlas({ geometryInstance });
    }
}

RenderSystem::RenderSystem(Scene &scene_, Camera &camera_,
        const RenderFunction &uiRenderFunction_)
    : scene(scene_)
    , camera(camera_)
    , uiRenderFunction(uiRenderFunction_)
{
    CreateRasterizationDescriptors();

    CreateRayTracingDescriptors();
    UpdateRayTracingDescriptors();


    frames.resize(VulkanContext::swapchain->GetImageViews().size());
    for (auto &frame : frames)
    {
        frame.commandBuffer = VulkanContext::device->AllocateCommandBuffer(CommandBufferType::eOneTime);
        frame.synchronization.waitSemaphores.push_back(VulkanHelpers::CreateSemaphore(VulkanContext::device->Get()));
        frame.synchronization.signalSemaphores.push_back(VulkanHelpers::CreateSemaphore(VulkanContext::device->Get()));
        frame.synchronization.fence = VulkanHelpers::CreateFence(VulkanContext::device->Get(),
                vk::FenceCreateFlagBits::eSignaled);
        switch (renderFlow)
        {
        case RenderFlow::eRasterization:
            frame.synchronization.waitStages.emplace_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
            break;
        case RenderFlow::eRayTracing:
            frame.synchronization.waitStages.emplace_back(vk::PipelineStageFlagBits::eRayTracingShaderNV);
            break;
        }
    }

    switch (renderFlow)
    {
    case RenderFlow::eRasterization:
        renderer = std::make_unique<Rasterizer>(scene, camera);
        break;
    case RenderFlow::eRayTracing:
        break;
    }

    drawingSuspended = false;
}

RenderSystem::~RenderSystem()
{
    for (auto &frame : frames)
    {
        VulkanHelpers::DestroyCommandBufferSync(VulkanContext::device->Get(), frame.synchronization);
    }
}

void RenderSystem::Process(float)
{
    if (drawingSuspended) return;

    const vk::SwapchainKHR swapchain = VulkanContext::swapchain->Get();
    const vk::Device device = VulkanContext::device->Get();

    const auto &[graphicsQueue, presentQueue] = VulkanContext::device->GetQueues();
    const auto &[commandBuffer, synchronization] = frames[frameIndex];

    const vk::Semaphore presentCompleteSemaphore = synchronization.waitSemaphores.front();
    const vk::Semaphore renderingCompleteSemaphore = synchronization.signalSemaphores.front();
    const vk::Fence renderingFence = synchronization.fence;

    const auto &[acquireResult, imageIndex] = VulkanContext::device->Get().acquireNextImageKHR(
            swapchain, Numbers::kMaxUint, presentCompleteSemaphore, nullptr);

    if (acquireResult == vk::Result::eErrorOutOfDateKHR) return;
    Assert(acquireResult == vk::Result::eSuccess || acquireResult == vk::Result::eSuboptimalKHR);

    VulkanHelpers::WaitForFences(VulkanContext::device->Get(), { renderingFence });

    const vk::Result resetResult = device.resetFences(1, &renderingFence);
    Assert(resetResult == vk::Result::eSuccess);

    const DeviceCommands renderCommands = std::bind(&RenderSystem::Render,
            this, std::placeholders::_1, imageIndex);

    VulkanHelpers::SubmitCommandBuffer(graphicsQueue, commandBuffer,
            renderCommands, synchronization);

    const vk::PresentInfoKHR presentInfo(1, &renderingCompleteSemaphore,
            1, &swapchain, &imageIndex, nullptr);

    const vk::Result presentResult = presentQueue.presentKHR(presentInfo);
    Assert(presentResult == vk::Result::eSuccess);

    frameIndex = (frameIndex + 1) % frames.size();
}

void RenderSystem::OnResize(const vk::Extent2D &extent)
{
    drawingSuspended = extent.width == 0 || extent.height == 0;

    if (!drawingSuspended)
    {
        renderer->OnResize(extent);

        UpdateRayTracingDescriptors();
    }
}

void RenderSystem::UpdateRayTracingResources(vk::CommandBuffer commandBuffer) const
{
    VulkanContext::bufferManager->UpdateBuffer(commandBuffer,
            rayTracingCameraBuffer, GetByteView(camera.GetData()));
}

void RenderSystem::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    renderer->Render(commandBuffer, imageIndex);
    uiRenderFunction(commandBuffer, imageIndex);
}

void RenderSystem::RayTrace(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    UpdateRayTracingResources(commandBuffer);

    const ImageLayoutTransition preRayTraceLayoutTransition{
        vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
        PipelineBarrier{
            SyncScope{
                frames.front().synchronization.waitStages.front(),
                vk::AccessFlags()
            },
            SyncScope::kRayTracingShaderWrite
        }
    };

    ImageHelpers::TransitImageLayout(commandBuffer, VulkanContext::swapchain->GetImages()[imageIndex],
            ImageHelpers::kFlatColor, preRayTraceLayoutTransition);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingNV, rayTracingPipeline->Get());

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingNV, rayTracingPipeline->GetLayout(),
            0, 1, &rayTracingDescriptors.descriptorSets[imageIndex], 0, nullptr);

    const ShaderBindingTable &shaderBindingTable = rayTracingPipeline->GetShaderBindingTable();
    const auto &[buffer, raygenOffset, missOffset, hitOffset, stride] = shaderBindingTable;

    const vk::Extent2D &extent = VulkanContext::swapchain->GetExtent();

    commandBuffer.traceRaysNV(buffer, raygenOffset,
            buffer, missOffset, stride,
            buffer, hitOffset, stride,
            nullptr, 0, 0,
            extent.width, extent.height, 1);

    const ImageLayoutTransition postRayTraceLayoutTransition{
        vk::ImageLayout::eGeneral, vk::ImageLayout::eColorAttachmentOptimal,
        PipelineBarrier{
            SyncScope::kRayTracingShaderWrite,
            SyncScope::kColorAttachmentWrite,
        }
    };

    ImageHelpers::TransitImageLayout(commandBuffer, VulkanContext::swapchain->GetImages()[imageIndex],
            ImageHelpers::kFlatColor, postRayTraceLayoutTransition);
}

void RenderSystem::CreateRasterizationDescriptors()
{
    texture = SRenderSystem::CreateTexture();
}

void RenderSystem::CreateRayTracingDescriptors()
{
    /*tlas = SRenderSystem::GenerateTlas(blas, glm::mat4(1.0f));
    rayTracingCameraBuffer = SRenderSystem::CreateRayTracingCameraBuffer();

    const uint32_t imageCount = static_cast<uint32_t>(VulkanContext::swapchain->GetImageViews().size());

    const DescriptorSetDescription description{
        { vk::DescriptorType::eAccelerationStructureNV, vk::ShaderStageFlagBits::eRaygenNV },
        { vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eRaygenNV },
        { vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eRaygenNV }
    };

    auto &[layout, descriptorSets] = rayTracingDescriptors;

    layout = VulkanContext::descriptorPool->CreateDescriptorSetLayout(description);
    descriptorSets = VulkanContext::descriptorPool->AllocateDescriptorSets(layout, imageCount);*/
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
