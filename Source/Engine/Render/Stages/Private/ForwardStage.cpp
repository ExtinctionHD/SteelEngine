#include "Engine/Render/Stages/ForwardStage.hpp"

#include "Engine/Engine.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Stages/GBufferStage.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/GlobalIllumination.hpp"
#include "Engine/Scene/Environment.hpp"
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
            0, vk::VertexInputRate::eVertex
        };

        const VertexDescription instanceDescription{
            { vk::Format::eR32G32B32A32Sfloat, vk::Format::eR32G32B32A32Sfloat },
            0, vk::VertexInputRate::eInstance
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

    static std::unique_ptr<GraphicsPipeline> CreateLightVolumePositionsPipeline(const RenderPass& renderPass,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
    {
        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eVertex,
                    Filepath("~/Shaders/Hybrid/LightVolumePositions.vert"), {}),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eFragment,
                    Filepath("~/Shaders/Hybrid/LightVolumePositions.frag"), {})
        };

        const VertexDescription vertexDescription{
            { vk::Format::eR32G32B32Sfloat },
            0, vk::VertexInputRate::eVertex
        };

        const VertexDescription instanceDescription{
            { vk::Format::eR32G32B32Sfloat },
            0, vk::VertexInputRate::eInstance
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

    static std::unique_ptr<GraphicsPipeline> CreateLightVolumeEdgesPipeline(const RenderPass& renderPass,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
    {
        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eVertex,
                    Filepath("~/Shaders/Hybrid/LightVolumeEdges.vert"), {}),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eFragment,
                    Filepath("~/Shaders/Hybrid/LightVolumeEdges.frag"), {})
        };

        const VertexDescription vertexDescription{
            { vk::Format::eR32G32B32Sfloat },
            0, vk::VertexInputRate::eVertex
        };

        const GraphicsPipeline::Description description{
            vk::PrimitiveTopology::eLineList,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            vk::FrontFace::eCounterClockwise,
            vk::SampleCountFlagBits::e1,
            vk::CompareOp::eLess,
            shaderModules,
            { vertexDescription },
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

ForwardStage::ForwardStage(const Scene* scene_, vk::ImageView depthImageView)
    : scene(scene_)
{
    renderPass = Details::CreateRenderPass();
    framebuffers = Details::CreateFramebuffers(*renderPass, depthImageView);

    SetupCameraData();
    SetupEnvironmentData();
    SetupPointLightsData();
    SetupLightVolumeData();

    SetupPipelines();

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &ForwardStage::HandleKeyInputEvent));
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

    if (scene->ctx().contains<LightVolumeComponent>())
    {
        DescriptorHelpers::DestroyDescriptorSet(lightVolumeData.positionsDescriptorSet);
        VulkanContext::bufferManager->DestroyBuffer(lightVolumeData.positionsIndexBuffer);
        VulkanContext::bufferManager->DestroyBuffer(lightVolumeData.positionsVertexBuffer);
        VulkanContext::bufferManager->DestroyBuffer(lightVolumeData.positionsInstanceBuffer);
        VulkanContext::bufferManager->DestroyBuffer(lightVolumeData.edgesIndexBuffer);
    }

    for (const auto& framebuffer : framebuffers)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }
}

void ForwardStage::Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const auto& cameraComponent = scene->ctx().at<CameraComponent>();

    const glm::mat4& view = cameraComponent.viewMatrix;
    const glm::mat4& proj = cameraComponent.projMatrix;

    const glm::mat4 defaultViewProj = proj * view;
    BufferHelpers::UpdateBuffer(commandBuffer, defaultCameraData.buffers[imageIndex],
            ByteView(defaultViewProj), SyncScope::kWaitForNone, SyncScope::kVertexUniformRead);

    const glm::mat4 environmentViewProj = proj * glm::mat4(glm::mat3(view));
    BufferHelpers::UpdateBuffer(commandBuffer, environmentCameraData.buffers[imageIndex],
            ByteView(environmentViewProj), SyncScope::kWaitForNone, SyncScope::kVertexUniformRead);

    const vk::Rect2D renderArea = RenderHelpers::GetSwapchainRenderArea();
    const std::vector<vk::ClearValue> clearValues = Details::GetClearValues();

    const vk::RenderPassBeginInfo beginInfo(
            renderPass->Get(), framebuffers[imageIndex],
            renderArea, clearValues);

    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

    if (pointLightsData.instanceCount > 0)
    {
        DrawPointLights(commandBuffer, imageIndex);
    }

    if (drawLightVolume)
    {
        DrawLightVolume(commandBuffer, imageIndex);
    }

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
    const uint32_t bufferCount = VulkanContext::swapchain->GetImageCount();

    constexpr vk::DeviceSize bufferSize = sizeof(glm::mat4);

    constexpr vk::ShaderStageFlags shaderStages = vk::ShaderStageFlagBits::eVertex;

    defaultCameraData = RenderHelpers::CreateCameraData(bufferCount, bufferSize, shaderStages);
    environmentCameraData = RenderHelpers::CreateCameraData(bufferCount, bufferSize, shaderStages);
}

void ForwardStage::SetupEnvironmentData()
{
    const auto& environmentComponent = scene->ctx().at<EnvironmentComponent>();

    const Texture& cubemapTexture = environmentComponent.cubemapTexture;

    environmentData.indexBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eIndexBuffer, ByteView(Details::kEnvironmentIndices));

    const DescriptorDescription descriptorDescription{
        1, vk::DescriptorType::eCombinedImageSampler,
        vk::ShaderStageFlagBits::eFragment,
        vk::DescriptorBindingFlags()
    };

    const DescriptorData descriptorData = DescriptorHelpers::GetData(
            RenderContext::defaultSampler, cubemapTexture.view);

    environmentData.descriptorSet = DescriptorHelpers::CreateDescriptorSet(
            { descriptorDescription }, { descriptorData });
}

void ForwardStage::SetupPointLightsData()
{
    /*const Scene::Hierarchy& sceneHierarchy = scene->GetHierarchy();

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
    }*/
}

void ForwardStage::SetupLightVolumeData()
{
    if (!scene->ctx().contains<LightVolumeComponent>())
    {
        return;
    }

    const auto& lightVolumeComponent = scene->ctx().at<LightVolumeComponent>();

    const Mesh sphere = MeshHelpers::GenerateSphere(Config::kLightProbeRadius);

    lightVolumeData.positionsIndexCount = static_cast<uint32_t>(sphere.indices.size());
    lightVolumeData.positionsInstanceCount = static_cast<uint32_t>(lightVolumeComponent.positions.size());
    lightVolumeData.edgesIndexCount = static_cast<uint32_t>(lightVolumeComponent.edgeIndices.size());

    lightVolumeData.positionsIndexBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eIndexBuffer, ByteView(sphere.indices));
    lightVolumeData.positionsVertexBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eVertexBuffer, ByteView(sphere.vertices));
    lightVolumeData.positionsInstanceBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eVertexBuffer, ByteView(lightVolumeComponent.positions));
    lightVolumeData.edgesIndexBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eIndexBuffer, ByteView(lightVolumeComponent.edgeIndices));

    const DescriptorDescription descriptorDescription{
        1, vk::DescriptorType::eStorageBuffer,
        vk::ShaderStageFlagBits::eFragment,
        vk::DescriptorBindingFlags()
    };

    const DescriptorData descriptorData
            = DescriptorHelpers::GetStorageData(lightVolumeComponent.coefficientsBuffer);

    lightVolumeData.positionsDescriptorSet = DescriptorHelpers::CreateDescriptorSet(
            { descriptorDescription }, { descriptorData });
}

void ForwardStage::DrawEnvironment(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Rect2D renderArea = RenderHelpers::GetSwapchainRenderArea();
    const vk::Viewport viewport = RenderHelpers::GetSwapchainViewport();

    const std::vector<vk::DescriptorSet> environmentDescriptorSets{
        environmentCameraData.descriptorSet.values[imageIndex],
        environmentData.descriptorSet.value
    };

    commandBuffer.setViewport(0, { viewport });
    commandBuffer.setScissor(0, { renderArea });

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, environmentPipeline->Get());

    commandBuffer.bindIndexBuffer(environmentData.indexBuffer, 0, vk::IndexType::eUint16);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            environmentPipeline->GetLayout(), 0, environmentDescriptorSets, {});

    commandBuffer.drawIndexed(Details::kEnvironmentIndexCount, 1, 0, 0, 0);

    commandBuffer.endRenderPass();
}

void ForwardStage::DrawPointLights(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Rect2D renderArea = RenderHelpers::GetSwapchainRenderArea();
    const vk::Viewport viewport = RenderHelpers::GetSwapchainViewport();

    const std::vector<vk::Buffer> vertexBuffers{
        pointLightsData.vertexBuffer,
        pointLightsData.instanceBuffer
    };

    const std::vector<vk::DescriptorSet> descriptorSets{
        defaultCameraData.descriptorSet.values[imageIndex]
    };

    commandBuffer.setViewport(0, { viewport });
    commandBuffer.setScissor(0, { renderArea });

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pointLightsPipeline->Get());

    commandBuffer.bindIndexBuffer(pointLightsData.indexBuffer, 0, vk::IndexType::eUint32);
    commandBuffer.bindVertexBuffers(0, vertexBuffers, { 0, 0 });

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            pointLightsPipeline->GetLayout(), 0, descriptorSets, {});

    commandBuffer.drawIndexed(pointLightsData.indexCount, pointLightsData.instanceCount, 0, 0, 0);
}

void ForwardStage::DrawLightVolume(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    if (!scene->ctx().contains<LightVolumeComponent>())
    {
        return;
    }

    const vk::Rect2D renderArea = RenderHelpers::GetSwapchainRenderArea();
    const vk::Viewport viewport = RenderHelpers::GetSwapchainViewport();

    const std::vector<vk::Buffer> positionsVertexBuffers{
        lightVolumeData.positionsVertexBuffer,
        lightVolumeData.positionsInstanceBuffer
    };

    const std::vector<vk::DescriptorSet> positionsDescriptorSets{
        defaultCameraData.descriptorSet.values[imageIndex],
        lightVolumeData.positionsDescriptorSet.value
    };

    commandBuffer.setViewport(0, { viewport });
    commandBuffer.setScissor(0, { renderArea });

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, lightVolumePositionsPipeline->Get());

    commandBuffer.bindIndexBuffer(lightVolumeData.positionsIndexBuffer, 0, vk::IndexType::eUint32);
    commandBuffer.bindVertexBuffers(0, positionsVertexBuffers, { 0, 0 });

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            lightVolumePositionsPipeline->GetLayout(), 0, positionsDescriptorSets, {});

    commandBuffer.drawIndexed(lightVolumeData.positionsIndexCount,
            lightVolumeData.positionsInstanceCount, 0, 0, 0);

    const std::vector<vk::DescriptorSet> edgesDescriptorSets{
        defaultCameraData.descriptorSet.values[imageIndex]
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, lightVolumeEdgesPipeline->Get());

    commandBuffer.bindIndexBuffer(lightVolumeData.edgesIndexBuffer, 0, vk::IndexType::eUint32);
    commandBuffer.bindVertexBuffers(0, { lightVolumeData.positionsInstanceBuffer }, { 0 });

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            lightVolumePositionsPipeline->GetLayout(), 0, edgesDescriptorSets, {});

    commandBuffer.drawIndexed(lightVolumeData.edgesIndexCount, 1, 0, 0, 0);
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

    if (scene->ctx().contains<LightVolumeComponent>())
    {
        const std::vector<vk::DescriptorSetLayout> lightVolumePositionsLayouts{
            defaultCameraData.descriptorSet.layout,
            lightVolumeData.positionsDescriptorSet.layout
        };

        lightVolumePositionsPipeline = Details::CreateLightVolumePositionsPipeline(
                *renderPass, lightVolumePositionsLayouts);

        const std::vector<vk::DescriptorSetLayout> lightVolumeEdgesLayouts{
            defaultCameraData.descriptorSet.layout
        };

        lightVolumeEdgesPipeline = Details::CreateLightVolumeEdgesPipeline(
                *renderPass, lightVolumeEdgesLayouts);
    }
}

void ForwardStage::HandleKeyInputEvent(const KeyInput& keyInput)
{
    if (keyInput.action == KeyAction::ePress)
    {
        switch (keyInput.key)
        {
        case Key::eV:
            drawLightVolume = !drawLightVolume;
            break;
        default:
            break;
        }
    }
}
