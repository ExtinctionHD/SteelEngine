#include "Engine/Render/Stages/ForwardStage.hpp"

#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Stages/GBufferStage.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Scene/GlobalIllumination.hpp"
#include "Engine/Scene/MeshHelpers.hpp"

namespace Details
{
    static const std::vector<uint16_t> kEnvironmentIndices{
        0, 3, 1,
        0, 2, 3,
        4, 2, 0,
        4, 6, 2,
        5, 6, 4,
        5, 7, 6,
        1, 7, 5,
        1, 3, 7,
        5, 0, 1,
        5, 4, 0,
        7, 3, 2,
        7, 2, 6
    };

    static const uint32_t kEnvironmentIndexCount = static_cast<uint32_t>(Details::kEnvironmentIndices.size());

    static std::unique_ptr<RenderPass> CreateRenderPass()
    {
        const std::vector<RenderPass::AttachmentDescription> attachments{
            RenderPass::AttachmentDescription{
                RenderPass::AttachmentUsage::eColor,
                VulkanContext::swapchain->GetFormat(),
                vk::AttachmentLoadOp::eLoad,
                vk::AttachmentStoreOp::eStore,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eColorAttachmentOptimal
            },
            RenderPass::AttachmentDescription{
                RenderPass::AttachmentUsage::eDepth,
                GBufferStage::kDepthFormat,
                vk::AttachmentLoadOp::eLoad,
                vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eDepthStencilAttachmentOptimal,
                vk::ImageLayout::eDepthStencilAttachmentOptimal
            }
        };

        const RenderPass::Description description{
            vk::PipelineBindPoint::eGraphics,
            vk::SampleCountFlagBits::e1, attachments
        };

        const std::vector<PipelineBarrier> previousDependencies{
            PipelineBarrier{
                SyncScope::kComputeShaderWrite,
                SyncScope::kColorAttachmentWrite
            },
            PipelineBarrier{
                SyncScope::kDepthStencilAttachmentWrite,
                SyncScope::kDepthStencilAttachmentRead
            },
        };

        const PipelineBarrier followingDependency{
            SyncScope::kColorAttachmentWrite,
            SyncScope::kColorAttachmentWrite
        };

        std::unique_ptr<RenderPass> renderPass = RenderPass::Create(description,
                RenderPass::Dependencies{ previousDependencies, { followingDependency } });

        return renderPass;
    }

    static std::vector<vk::Framebuffer> CreateFramebuffers(const RenderPass& renderPass, vk::ImageView depthImageView)
    {
        const vk::Device device = VulkanContext::device->Get();

        const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

        const std::vector<vk::ImageView>& swapchainImageViews = VulkanContext::swapchain->GetImageViews();

        return VulkanHelpers::CreateFramebuffers(device, renderPass.Get(),
                extent, { swapchainImageViews }, { depthImageView });
    }

    static std::unique_ptr<GraphicsPipeline> CreateEnvironmentPipeline(const RenderPass& renderPass,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
    {
        constexpr int32_t reverseDepth = static_cast<int32_t>(Config::kReverseDepth);

        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eVertex,
                    Filepath("~/Shaders/Hybrid/Environment.vert"),
                    { std::make_pair("REVERSE_DEPTH", reverseDepth) }),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eFragment,
                    Filepath("~/Shaders/Hybrid/Environment.frag"), {})
        };

        const GraphicsPipeline::Description description{
            vk::PrimitiveTopology::eTriangleList,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eFront,
            vk::FrontFace::eCounterClockwise,
            vk::SampleCountFlagBits::e1,
            vk::CompareOp::eLessOrEqual,
            shaderModules,
            {},
            { BlendMode::eDisabled },
            descriptorSetLayouts,
            {}
        };

        std::unique_ptr<GraphicsPipeline> pipeline = GraphicsPipeline::Create(renderPass.Get(), description);

        for (const auto& shaderModule : shaderModules)
        {
            VulkanContext::shaderManager->DestroyShaderModule(shaderModule);
        }

        return pipeline;
    }

    static std::unique_ptr<GraphicsPipeline> CreatePointLightsPipeline(const RenderPass& renderPass,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
    {
        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eVertex,
                    Filepath("~/Shaders/Hybrid/PointLights.vert"), {}),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eFragment,
                    Filepath("~/Shaders/Hybrid/PointLights.frag"), {})
        };

        const VertexDescription vertexDescription{
            { vk::Format::eR32G32B32Sfloat },
            vk::VertexInputRate::eVertex
        };

        const VertexDescription instanceDescription{
            { vk::Format::eR32G32B32A32Sfloat, vk::Format::eR32G32B32A32Sfloat },
            vk::VertexInputRate::eInstance
        };

        const GraphicsPipeline::Description description{
            vk::PrimitiveTopology::eTriangleList,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            vk::FrontFace::eCounterClockwise,
            vk::SampleCountFlagBits::e1,
            vk::CompareOp::eLess,
            shaderModules,
            { vertexDescription, instanceDescription },
            { BlendMode::eDisabled },
            descriptorSetLayouts,
            {}
        };

        std::unique_ptr<GraphicsPipeline> pipeline = GraphicsPipeline::Create(renderPass.Get(), description);

        for (const auto& shaderModule : shaderModules)
        {
            VulkanContext::shaderManager->DestroyShaderModule(shaderModule);
        }

        return pipeline;
    }

    static std::unique_ptr<GraphicsPipeline> CreateIrradianceVolumePipeline(const RenderPass& renderPass,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
    {
        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eVertex,
                    Filepath("~/Shaders/Hybrid/IrradianceVolume.vert"), {}),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eFragment,
                    Filepath("~/Shaders/Hybrid/IrradianceVolume.frag"), {})
        };

        const VertexDescription vertexDescription{
            { vk::Format::eR32G32B32Sfloat },
            vk::VertexInputRate::eVertex
        };

        const VertexDescription instanceDescription{
            { vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Uint },
            vk::VertexInputRate::eInstance
        };

        const GraphicsPipeline::Description description{
            vk::PrimitiveTopology::eTriangleList,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            vk::FrontFace::eCounterClockwise,
            vk::SampleCountFlagBits::e1,
            vk::CompareOp::eLess,
            shaderModules,
            { vertexDescription, instanceDescription },
            { BlendMode::eDisabled },
            descriptorSetLayouts,
            {}
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
        return { VulkanHelpers::kDefaultClearColorValue, VulkanHelpers::kDefaultClearDepthStencilValue };
    }
}

ForwardStage::ForwardStage(Scene* scene_, Camera* camera_, Environment* environment_,
        IrradianceVolume* irradianceVolume_, vk::ImageView depthImageView)
    : scene(scene_)
    , camera(camera_)
    , environment(environment_)
    , irradianceVolume(irradianceVolume_)
{
    renderPass = Details::CreateRenderPass();
    framebuffers = Details::CreateFramebuffers(*renderPass, depthImageView);

    SetupCameraData();
    SetupEnvironmentData();
    SetupPointLightsData();
    SetupIrradianceVolumeData();

    SetupPipelines();
}

ForwardStage::~ForwardStage()
{
    DescriptorHelpers::DestroyMultiDescriptorSet(defaultCameraData.descriptorSet);
    for (const auto& buffer : defaultCameraData.buffers)
    {
        VulkanContext::bufferManager->DestroyBuffer(buffer);
    }

    DescriptorHelpers::DestroyMultiDescriptorSet(environmentCameraData.descriptorSet);
    for (const auto& buffer : environmentCameraData.buffers)
    {
        VulkanContext::bufferManager->DestroyBuffer(buffer);
    }

    DescriptorHelpers::DestroyDescriptorSet(environmentData.descriptorSet);
    VulkanContext::bufferManager->DestroyBuffer(environmentData.indexBuffer);

    if (pointLightsData.instanceCount > 0)
    {
        VulkanContext::bufferManager->DestroyBuffer(pointLightsData.indexBuffer);
        VulkanContext::bufferManager->DestroyBuffer(pointLightsData.vertexBuffer);
        VulkanContext::bufferManager->DestroyBuffer(pointLightsData.instanceBuffer);
    }

    DescriptorHelpers::DestroyDescriptorSet(irradianceVolumeData.descriptorSet);
    VulkanContext::bufferManager->DestroyBuffer(irradianceVolumeData.indexBuffer);
    VulkanContext::bufferManager->DestroyBuffer(irradianceVolumeData.vertexBuffer);
    VulkanContext::bufferManager->DestroyBuffer(irradianceVolumeData.instanceBuffer);
    VulkanContext::textureManager->DestroySampler(irradianceVolumeData.sampler);

    for (const auto& framebuffer : framebuffers)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }
}

void ForwardStage::Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const glm::mat4& view = camera->GetViewMatrix();
    const glm::mat4& proj = camera->GetProjectionMatrix();

    const glm::mat4 defaultViewProj = proj * view;
    BufferHelpers::UpdateBuffer(commandBuffer, defaultCameraData.buffers[imageIndex],
            ByteView(defaultViewProj), SyncScope::kWaitForNone, SyncScope::kVertexUniformRead);

    const glm::mat4 environmentViewProj = proj * glm::mat4(glm::mat3(view));
    BufferHelpers::UpdateBuffer(commandBuffer, environmentCameraData.buffers[imageIndex],
            ByteView(environmentViewProj), SyncScope::kWaitForNone, SyncScope::kVertexUniformRead);

    const vk::Rect2D renderArea = StageHelpers::GetSwapchainRenderArea();
    const std::vector<vk::ClearValue> clearValues = Details::GetClearValues();

    const vk::RenderPassBeginInfo beginInfo(
            renderPass->Get(), framebuffers[imageIndex],
            renderArea, clearValues);

    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

    if (pointLightsData.instanceCount > 0)
    {
        DrawPointLights(commandBuffer, imageIndex);
    }

    DrawIrradianceVolume(commandBuffer, imageIndex);

    DrawEnvironment(commandBuffer, imageIndex);
}

void ForwardStage::Resize(vk::ImageView depthImageView)
{
    for (const auto& framebuffer : framebuffers)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }

    renderPass = Details::CreateRenderPass();
    framebuffers = Details::CreateFramebuffers(*renderPass, depthImageView);
}

void ForwardStage::ReloadShaders()
{
    SetupPipelines();
}

void ForwardStage::SetupCameraData()
{
    const size_t bufferCount = VulkanContext::swapchain->GetImages().size();

    constexpr vk::DeviceSize bufferSize = sizeof(glm::mat4);

    constexpr vk::ShaderStageFlags shaderStages = vk::ShaderStageFlagBits::eVertex;

    defaultCameraData = StageHelpers::CreateCameraData(bufferCount, bufferSize, shaderStages);
    environmentCameraData = StageHelpers::CreateCameraData(bufferCount, bufferSize, shaderStages);
}

void ForwardStage::SetupEnvironmentData()
{
    environmentData.indexBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eIndexBuffer, ByteView(Details::kEnvironmentIndices));

    const DescriptorDescription descriptorDescription{
        1, vk::DescriptorType::eCombinedImageSampler,
        vk::ShaderStageFlagBits::eFragment,
        vk::DescriptorBindingFlags()
    };

    const DescriptorData descriptorData = DescriptorHelpers::GetData(
            RenderContext::defaultSampler, environment->GetTexture().view);

    environmentData.descriptorSet = DescriptorHelpers::CreateDescriptorSet(
            { descriptorDescription }, { descriptorData });
}

void ForwardStage::SetupPointLightsData()
{
    const Scene::Hierarchy& sceneHierarchy = scene->GetHierarchy();

    if (!sceneHierarchy.pointLights.empty())
    {
        const Mesh sphere = MeshHelpers::GenerateSphere(Config::kPointLightRadius);

        pointLightsData.indexCount = static_cast<uint32_t>(sphere.indices.size());
        pointLightsData.instanceCount = static_cast<uint32_t>(sceneHierarchy.pointLights.size());

        pointLightsData.indexBuffer = BufferHelpers::CreateBufferWithData(
                vk::BufferUsageFlagBits::eIndexBuffer, ByteView(sphere.indices));
        pointLightsData.vertexBuffer = BufferHelpers::CreateBufferWithData(
                vk::BufferUsageFlagBits::eVertexBuffer, ByteView(sphere.vertices));
        pointLightsData.instanceBuffer = BufferHelpers::CreateBufferWithData(
                vk::BufferUsageFlagBits::eVertexBuffer, ByteView(sceneHierarchy.pointLights));
    }
}

void ForwardStage::SetupIrradianceVolumeData()
{
    Assert(!irradianceVolume->points.empty());

    const Mesh sphere = MeshHelpers::GenerateSphere(Config::kIrradianceVolumePointRadius);

    irradianceVolumeData.indexCount = static_cast<uint32_t>(sphere.indices.size());
    irradianceVolumeData.instanceCount = static_cast<uint32_t>(irradianceVolume->points.size());

    irradianceVolumeData.indexBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eIndexBuffer, ByteView(sphere.indices));
    irradianceVolumeData.vertexBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eVertexBuffer, ByteView(sphere.vertices));

    irradianceVolumeData.instanceBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eVertexBuffer, ByteView(irradianceVolume->points));

    const SamplerDescription samplerDescription{
        vk::Filter::eNearest,
        vk::Filter::eNearest,
        vk::SamplerMipmapMode::eNearest,
        vk::SamplerAddressMode::eClampToEdge,
        std::nullopt, 0.0f, 0.0f, true
    };

    irradianceVolumeData.sampler = VulkanContext::textureManager->CreateSampler(samplerDescription);

    const DescriptorDescription descriptorDescription{
        static_cast<uint32_t>(irradianceVolume->textures.size()),
        vk::DescriptorType::eCombinedImageSampler,
        vk::ShaderStageFlagBits::eFragment,
        vk::DescriptorBindingFlags()
    };

    ImageInfo irradianceVolumeDescriptorInfo;
    for (const auto& texture : irradianceVolume->textures)
    {
        irradianceVolumeDescriptorInfo.emplace_back(irradianceVolumeData.sampler,
                texture.view, vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    const DescriptorData descriptorData{
        vk::DescriptorType::eCombinedImageSampler,
        irradianceVolumeDescriptorInfo
    };

    irradianceVolumeData.descriptorSet = DescriptorHelpers::CreateDescriptorSet(
            { descriptorDescription }, { descriptorData });
}

void ForwardStage::DrawEnvironment(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Rect2D renderArea = StageHelpers::GetSwapchainRenderArea();
    const vk::Viewport viewport = StageHelpers::GetSwapchainViewport();

    const std::vector<vk::DescriptorSet> environmentDescriptorSets{
        environmentCameraData.descriptorSet.values[imageIndex],
        environmentData.descriptorSet.value
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, environmentPipeline->Get());

    commandBuffer.setViewport(0, { viewport });
    commandBuffer.setScissor(0, { renderArea });

    commandBuffer.bindIndexBuffer(environmentData.indexBuffer, 0, vk::IndexType::eUint16);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            environmentPipeline->GetLayout(), 0, environmentDescriptorSets, {});

    commandBuffer.drawIndexed(Details::kEnvironmentIndexCount, 1, 0, 0, 0);

    commandBuffer.endRenderPass();
}

void ForwardStage::DrawPointLights(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Rect2D renderArea = StageHelpers::GetSwapchainRenderArea();
    const vk::Viewport viewport = StageHelpers::GetSwapchainViewport();

    const std::vector<vk::Buffer> vertexBuffers{
        pointLightsData.vertexBuffer,
        pointLightsData.instanceBuffer
    };

    const std::vector<vk::DescriptorSet> descriptorSets{
        defaultCameraData.descriptorSet.values[imageIndex]
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pointLightsPipeline->Get());

    commandBuffer.setViewport(0, { viewport });
    commandBuffer.setScissor(0, { renderArea });

    commandBuffer.bindIndexBuffer(pointLightsData.indexBuffer, 0, vk::IndexType::eUint32);
    commandBuffer.bindVertexBuffers(0, vertexBuffers, { 0, 0 });

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            pointLightsPipeline->GetLayout(), 0, descriptorSets, {});

    commandBuffer.drawIndexed(pointLightsData.indexCount, pointLightsData.instanceCount, 0, 0, 0);
}

void ForwardStage::DrawIrradianceVolume(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Rect2D renderArea = StageHelpers::GetSwapchainRenderArea();
    const vk::Viewport viewport = StageHelpers::GetSwapchainViewport();

    const std::vector<vk::Buffer> vertexBuffers{
        irradianceVolumeData.vertexBuffer,
        irradianceVolumeData.instanceBuffer
    };

    const std::vector<vk::DescriptorSet> descriptorSets{
        defaultCameraData.descriptorSet.values[imageIndex],
        irradianceVolumeData.descriptorSet.value
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, irradianceVolumePipeline->Get());

    commandBuffer.setViewport(0, { viewport });
    commandBuffer.setScissor(0, { renderArea });

    commandBuffer.bindIndexBuffer(irradianceVolumeData.indexBuffer, 0, vk::IndexType::eUint32);
    commandBuffer.bindVertexBuffers(0, vertexBuffers, { 0, 0 });

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            irradianceVolumePipeline->GetLayout(), 0, descriptorSets, {});

    commandBuffer.drawIndexed(irradianceVolumeData.indexCount, irradianceVolumeData.instanceCount, 0, 0, 0);
}

void ForwardStage::SetupPipelines()
{
    const std::vector<vk::DescriptorSetLayout> environmentLayouts{
        environmentCameraData.descriptorSet.layout,
        environmentData.descriptorSet.layout
    };

    environmentPipeline = Details::CreateEnvironmentPipeline(*renderPass, environmentLayouts);

    if (pointLightsData.instanceCount > 0)
    {
        const std::vector<vk::DescriptorSetLayout> pointLightsLayouts{
            defaultCameraData.descriptorSet.layout
        };

        pointLightsPipeline = Details::CreatePointLightsPipeline(*renderPass, pointLightsLayouts);
    }

    const std::vector<vk::DescriptorSetLayout> irradianceVolumeLayouts{
        defaultCameraData.descriptorSet.layout,
        irradianceVolumeData.descriptorSet.layout
    };

    irradianceVolumePipeline = Details::CreateIrradianceVolumePipeline(*renderPass, irradianceVolumeLayouts);
}
