#include "Engine/System/RenderSystem.hpp"


#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Engine.hpp"
#include "Engine/EngineHelpers.hpp"
#include "Engine/InputHelpers.hpp"

namespace Details
{
    constexpr vk::Format kDepthFormat = vk::Format::eD32Sfloat;

    std::unique_ptr<RenderPass> CreateForwardRenderPass()
    {
        const std::vector<RenderPass::AttachmentDescription> attachments{
            RenderPass::AttachmentDescription{
                RenderPass::AttachmentUsage::eColor,
                VulkanContext::swapchain->GetFormat(),
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                vk::ImageLayout::ePresentSrcKHR,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eColorAttachmentOptimal
            },
            RenderPass::AttachmentDescription{
                RenderPass::AttachmentUsage::eDepth,
                kDepthFormat,
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eDepthStencilAttachmentOptimal,
                vk::ImageLayout::eDepthStencilAttachmentOptimal,
                vk::ImageLayout::eDepthStencilAttachmentOptimal
            }
        };

        const RenderPass::Description description{
            vk::PipelineBindPoint::eGraphics,
            vk::SampleCountFlagBits::e1,
            attachments
        };

        const PipelineBarrier pipelineBarrier{
            SyncScope::kColorAttachmentWrite,
            SyncScope::kColorAttachmentWrite
        };

        std::unique_ptr<RenderPass> renderPass = RenderPass::Create(description,
                RenderPass::Dependencies{ std::nullopt, pipelineBarrier });

        return renderPass;
    }

    std::unique_ptr<GraphicsPipeline> CreateDefaultPipeline(const RenderPass& renderPass,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
    {
        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eVertex,
                    Filepath("~/Shaders/Forward/Default.vert")),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eFragment,
                    Filepath("~/Shaders/Forward/Default.frag"))
        };

        const VertexDescription vertexDescription{
            Scene::Mesh::Vertex::kFormat,
            vk::VertexInputRate::eVertex
        };

        const vk::PushConstantRange pushConstantRange{
            vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4)
        };

        const GraphicsPipeline::Description description{
            VulkanContext::swapchain->GetExtent(),
            vk::PrimitiveTopology::eTriangleList,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            vk::FrontFace::eCounterClockwise,
            vk::SampleCountFlagBits::e1,
            std::nullopt,
            shaderModules,
            { vertexDescription },
            { BlendMode::eDisabled },
            descriptorSetLayouts,
            { pushConstantRange }
        };

        std::unique_ptr<GraphicsPipeline> pipeline = GraphicsPipeline::Create(renderPass.Get(), description);

        for (const auto& shaderModule : shaderModules)
        {
            VulkanContext::shaderManager->DestroyShaderModule(shaderModule);
        }

        return pipeline;
    }

    std::unique_ptr<GraphicsPipeline> CreateEnvironmentPipeline(const RenderPass& renderPass,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
    {
        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eVertex,
                    Filepath("~/Shaders/Forward/Environment.vert")),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eFragment,
                    Filepath("~/Shaders/Forward/Environment.frag"))
        };

        const vk::PushConstantRange pushConstantRange{
            vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::vec3)
        };

        const GraphicsPipeline::Description description{
            VulkanContext::swapchain->GetExtent(),
            vk::PrimitiveTopology::eTriangleStrip,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eFront,
            vk::FrontFace::eCounterClockwise,
            vk::SampleCountFlagBits::e1,
            vk::CompareOp::eLessOrEqual,
            shaderModules,
            {},
            { BlendMode::eDisabled },
            descriptorSetLayouts,
            { pushConstantRange }
        };

        std::unique_ptr<GraphicsPipeline> pipeline = GraphicsPipeline::Create(renderPass.Get(), description);

        for (const auto& shaderModule : shaderModules)
        {
            VulkanContext::shaderManager->DestroyShaderModule(shaderModule);
        }

        return pipeline;
    }
}

RenderSystem::RenderSystem(Scene* scene_, Camera* camera_, Environment* environment_)
    : scene(scene_)
    , camera(camera_)
    , environment(environment_)
{
    SetupCamera();
    SetupEnvironment();

    forwardRenderPass = Details::CreateForwardRenderPass();

    SetupPipelines();
    SetupDepthAttachments();
    SetupFramebuffers();

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &RenderSystem::HandleResizeEvent));

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &RenderSystem::HandleKeyInputEvent));
}

RenderSystem::~RenderSystem()
{
    for (const auto& framebuffer : framebuffers)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }

    for (const auto& depthAttachment : depthAttachments)
    {
        VulkanContext::imageManager->DestroyImage(depthAttachment.image);
    }
}

void RenderSystem::Process(float) {}

void RenderSystem::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    UpdateCameraBuffer(commandBuffer);

    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    const vk::Rect2D renderArea(vk::Offset2D(0, 0), extent);

    const std::vector<vk::ClearValue> clearValues{
        vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }),
        vk::ClearDepthStencilValue(1.0f, 0)
    };

    const vk::RenderPassBeginInfo beginInfo(
            forwardRenderPass->Get(), framebuffers[imageIndex], renderArea,
            static_cast<uint32_t>(clearValues.size()), clearValues.data());

    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

    DrawEnvironment(commandBuffer);

    DrawRenderObjects(commandBuffer);

    commandBuffer.endRenderPass();
}

void RenderSystem::SetupCamera()
{
    const BufferDescription bufferDescription{
        sizeof(glm::mat4),
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    cameraData.buffer = VulkanContext::bufferManager->CreateBuffer(
            bufferDescription, BufferCreateFlagBits::eStagingBuffer);

    const DescriptorDescription descriptorDescription{
        1, vk::DescriptorType::eUniformBuffer,
        vk::ShaderStageFlagBits::eVertex,
        vk::DescriptorBindingFlags()
    };

    const DescriptorData descriptorData = DescriptorHelpers::GetData(cameraData.buffer);

    cameraData.descriptorSet = DescriptorHelpers::CreateDescriptorSet(
            { descriptorDescription }, { descriptorData });
}

void RenderSystem::SetupEnvironment()
{
    const vk::Sampler sampler = VulkanContext::textureManager->GetDefaultSampler();

    const DescriptorDescription descriptorDescription{
        1, vk::DescriptorType::eCombinedImageSampler,
        vk::ShaderStageFlagBits::eFragment,
        vk::DescriptorBindingFlags()
    };

    const DescriptorData descriptorData = DescriptorHelpers::GetData(sampler, environment->GetTexture().view);

    environmentData.descriptorSet = DescriptorHelpers::CreateDescriptorSet(
            { descriptorDescription }, { descriptorData });
}

void RenderSystem::SetupPipelines()
{
    const std::vector<vk::DescriptorSetLayout> defaultPipelineLayouts{
        cameraData.descriptorSet.layout,
        scene->GetDescriptorSets().materials.layout
    };

    const std::vector<vk::DescriptorSetLayout> environmentPipelineLayouts{
        cameraData.descriptorSet.layout,
        environmentData.descriptorSet.layout
    };

    defaultPipeline = Details::CreateDefaultPipeline(*forwardRenderPass, defaultPipelineLayouts);
    environmentPipeline = Details::CreateEnvironmentPipeline(*forwardRenderPass, environmentPipelineLayouts);
}

void RenderSystem::SetupDepthAttachments()
{
    depthAttachments.resize(VulkanContext::swapchain->GetImages().size());

    const vk::Extent3D extent = VulkanHelpers::GetExtent3D(VulkanContext::swapchain->GetExtent());

    for (auto& depthAttachment : depthAttachments)
    {
        const ImageDescription imageDescription{
            ImageType::e2D,
            Details::kDepthFormat,
            extent, 1, 1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::ImageLayout::eUndefined,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        depthAttachment.image = VulkanContext::imageManager->CreateImage(
                imageDescription, ImageCreateFlags::kNone);

        depthAttachment.view = VulkanContext::imageManager->CreateView(
                depthAttachment.image, vk::ImageViewType::e2D, ImageHelpers::kFlatDepth);
    }

    VulkanContext::device->ExecuteOneTimeCommands([this](vk::CommandBuffer commandBuffer)
        {
            for (const auto& depthAttachment : depthAttachments)
            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    PipelineBarrier{
                        SyncScope::kWaitForNone,
                        SyncScope::kBlockAll
                    }
                };

                ImageHelpers::TransitImageLayout(commandBuffer, depthAttachment.image, ImageHelpers::kFlatDepth,
                        layoutTransition);
            }
        });
}

void RenderSystem::SetupFramebuffers()
{
    const vk::Device device = VulkanContext::device->Get();
    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    const std::vector<vk::ImageView>& swapchainImageViews = VulkanContext::swapchain->GetImageViews();

    std::vector<vk::ImageView> depthImageViews;
    depthImageViews.reserve(depthAttachments.size());

    for (const auto& depthAttachment : depthAttachments)
    {
        depthImageViews.push_back(depthAttachment.view);
    }

    framebuffers = VulkanHelpers::CreateFramebuffers(device, forwardRenderPass->Get(),
            extent, { swapchainImageViews, depthImageViews }, {});
}

void RenderSystem::UpdateCameraBuffer(vk::CommandBuffer commandBuffer) const
{
    const glm::mat4 viewProj = camera->GetProjectionMatrix() * camera->GetViewMatrix();

    BufferHelpers::UpdateBuffer(commandBuffer, cameraData.buffer,
            ByteView(viewProj), SyncScope::kVertexShaderRead);
}

void RenderSystem::DrawEnvironment(vk::CommandBuffer commandBuffer) const
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, environmentPipeline->Get());

    const std::vector<vk::DescriptorSet> descriptorSets{
        cameraData.descriptorSet.value,
        environmentData.descriptorSet.value
    };

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            environmentPipeline->GetLayout(), 0, descriptorSets, {});

    const glm::vec3 cameraPosition = camera->GetDescription().position;

    commandBuffer.pushConstants<glm::vec3>(environmentPipeline->GetLayout(),
            vk::ShaderStageFlagBits::eVertex, 0, { cameraPosition });

    commandBuffer.draw(48, 1, 0, 0);
}

void RenderSystem::DrawRenderObjects(vk::CommandBuffer) const
{ }

void RenderSystem::HandleResizeEvent(const vk::Extent2D& extent)
{
    if (extent.width != 0 && extent.height != 0)
    { }
}

void RenderSystem::HandleKeyInputEvent(const KeyInput& keyInput)
{
    if (keyInput.action == KeyAction::ePress)
    {
        switch (keyInput.key)
        {
        case Key::eR:
            ReloadShaders();
            break;
        default:
            break;
        }
    }
}

void RenderSystem::ReloadShaders()
{
    VulkanContext::device->WaitIdle();
}
