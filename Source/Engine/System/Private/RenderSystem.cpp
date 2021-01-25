#include "Engine/System/RenderSystem.hpp"

#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Engine.hpp"
#include "Engine/EngineHelpers.hpp"
#include "Engine/InputHelpers.hpp"
#include "Engine/Render/Renderer.hpp"

namespace Details
{
    static constexpr vk::Format kDepthFormat = vk::Format::eD32Sfloat;

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

    static std::unique_ptr<RenderPass> CreateForwardRenderPass()
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

    static std::unique_ptr<GraphicsPipeline> CreateHybridPipeline(const RenderPass& renderPass,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts, const Scene::PipelineState& pipelineState)
    {
        const std::map<std::string, uint32_t> defines{
            { "ALPHA_TEST", static_cast<uint32_t>(pipelineState.alphaTest) },
            { "DOUBLE_SIDED", static_cast<uint32_t>(pipelineState.doubleSided) }
        };

        const vk::CullModeFlagBits cullMode = pipelineState.doubleSided
                ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack;

        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eVertex,
                    Filepath("~/Shaders/Hybrid/Hybrid.vert"), defines),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eFragment,
                    Filepath("~/Shaders/Hybrid/Hybrid.frag"), defines)
        };

        const VertexDescription vertexDescription{
            Scene::Mesh::Vertex::kFormat,
            vk::VertexInputRate::eVertex
        };

        const vk::PushConstantRange pushConstantRange(
                vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4));

        const GraphicsPipeline::Description description{
            VulkanContext::swapchain->GetExtent(),
            vk::PrimitiveTopology::eTriangleList,
            vk::PolygonMode::eFill,
            cullMode,
            vk::FrontFace::eCounterClockwise,
            vk::SampleCountFlagBits::e1,
            vk::CompareOp::eLess,
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

    static std::unique_ptr<GraphicsPipeline> CreateEnvironmentPipeline(const RenderPass& renderPass,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
    {
        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eVertex,
                    Filepath("~/Shaders/Hybrid/Environment.vert"), {}),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eFragment,
                    Filepath("~/Shaders/Hybrid/Environment.frag"), {})
        };

        const GraphicsPipeline::Description description{
            VulkanContext::swapchain->GetExtent(),
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
}

RenderSystem::RenderSystem(Scene* scene_, Camera* camera_, Environment* environment_)
    : scene(scene_)
    , camera(camera_)
    , environment(environment_)
{
    SetupCameraData();
    SetupLightingData();
    SetupEnvironmentData();

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
    VulkanContext::bufferManager->DestroyBuffer(cameraData.viewProjBuffer);
    VulkanContext::bufferManager->DestroyBuffer(cameraData.cameraPositionBuffer);
    DescriptorHelpers::DestroyDescriptorSet(cameraData.descriptorSet);

    VulkanContext::bufferManager->DestroyBuffer(lightingData.buffer);
    DescriptorHelpers::DestroyDescriptorSet(lightingData.descriptorSet);

    VulkanContext::bufferManager->DestroyBuffer(environmentData.indexBuffer);
    VulkanContext::bufferManager->DestroyBuffer(environmentData.viewProjBuffer);
    DescriptorHelpers::DestroyDescriptorSet(environmentData.descriptorSet);

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
    UpdateCameraBuffers(commandBuffer);

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

    DrawScene(commandBuffer);

    commandBuffer.endRenderPass();
}

void RenderSystem::SetupCameraData()
{
    const BufferDescription viewProjBufferDescription{
        sizeof(glm::mat4),
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    cameraData.viewProjBuffer = VulkanContext::bufferManager->CreateBuffer(
            viewProjBufferDescription, BufferCreateFlagBits::eStagingBuffer);

    const BufferDescription cameraPositionBufferDescription{
        sizeof(glm::vec3),
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    cameraData.cameraPositionBuffer = VulkanContext::bufferManager->CreateBuffer(
            cameraPositionBufferDescription, BufferCreateFlagBits::eStagingBuffer);

    const DescriptorSetDescription descriptorSetDescription{
        DescriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eVertex,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eFragment,
            vk::DescriptorBindingFlags()
        }
    };

    const DescriptorSetData descriptorSetData{
        DescriptorHelpers::GetData(cameraData.viewProjBuffer),
        DescriptorHelpers::GetData(cameraData.cameraPositionBuffer)
    };

    cameraData.descriptorSet = DescriptorHelpers::CreateDescriptorSet(
            descriptorSetDescription, descriptorSetData);
}

void RenderSystem::SetupLightingData()
{
    const Texture& irradianceTexture = environment->GetIrradianceTexture();
    const Texture& reflectionTexture = environment->GetReflectionTexture();
    const Texture& specularBRDF = Renderer::imageBasedLighting->GetSpecularBRDF();

    const ImageBasedLighting::Samplers& iblSamplers = Renderer::imageBasedLighting->GetSamplers();

    const DirectLight& directLight = environment->GetDirectLight();

    lightingData.buffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eUniformBuffer, ByteView(directLight));

    const DescriptorSetDescription descriptorSetDescription{
        DescriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eFragment,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eFragment,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eFragment,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eFragment,
            vk::DescriptorBindingFlags()
        },
    };

    const DescriptorSetData descriptorSetData{
        DescriptorHelpers::GetData(iblSamplers.irradiance, irradianceTexture.view),
        DescriptorHelpers::GetData(iblSamplers.reflection, reflectionTexture.view),
        DescriptorHelpers::GetData(iblSamplers.specularBRDF, specularBRDF.view),
        DescriptorHelpers::GetData(lightingData.buffer),
    };

    lightingData.descriptorSet = DescriptorHelpers::CreateDescriptorSet(
            descriptorSetDescription, descriptorSetData);
}

void RenderSystem::SetupEnvironmentData()
{
    environmentData.indexBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eIndexBuffer, ByteView(Details::kEnvironmentIndices));

    const BufferDescription viewProjBufferDescription{
        sizeof(glm::mat4),
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    environmentData.viewProjBuffer = VulkanContext::bufferManager->CreateBuffer(
            viewProjBufferDescription, BufferCreateFlagBits::eStagingBuffer);

    const DescriptorSetDescription descriptorSetDescription{
        DescriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eVertex,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eFragment,
            vk::DescriptorBindingFlags()
        }
    };

    const DescriptorSetData descriptorSetData{
        DescriptorHelpers::GetData(environmentData.viewProjBuffer),
        DescriptorHelpers::GetData(Renderer::defaultSampler, environment->GetTexture().view)
    };

    environmentData.descriptorSet = DescriptorHelpers::CreateDescriptorSet(
            descriptorSetDescription, descriptorSetData);
}

void RenderSystem::SetupPipelines()
{
    scenePipelines.clear();

    const std::vector<vk::DescriptorSetLayout> scenePipelineLayouts{
        cameraData.descriptorSet.layout,
        lightingData.descriptorSet.layout,
        scene->GetDescriptorSets().rayTracing.layout,
        scene->GetDescriptorSets().materials.layout
    };

    const std::vector<vk::DescriptorSetLayout> environmentPipelineLayouts{
        environmentData.descriptorSet.layout
    };

    const Scene::Hierarchy& sceneHierarchy = scene->GetHierarchy();

    for (uint32_t i = 0; i < static_cast<uint32_t>(sceneHierarchy.materials.size()); ++i)
    {
        const Scene::Material& material = sceneHierarchy.materials[i];

        const auto pred = [&material](const ScenePipeline& scenePipeline)
            {
                return scenePipeline.state == material.pipelineState;
            };

        const auto it = std::find_if(scenePipelines.begin(), scenePipelines.end(), pred);

        if (it != scenePipelines.end())
        {
            it->materialIndices.push_back(i);
        }
        else
        {
            std::unique_ptr<GraphicsPipeline> pipeline = Details::CreateHybridPipeline(
                    *forwardRenderPass, scenePipelineLayouts, material.pipelineState);

            scenePipelines.push_back(ScenePipeline{ material.pipelineState, std::move(pipeline), { i } });
        }
    }

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
                        SyncScope::kBlockNone
                    }
                };

                ImageHelpers::TransitImageLayout(commandBuffer,
                        depthAttachment.image, ImageHelpers::kFlatDepth, layoutTransition);
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

void RenderSystem::UpdateCameraBuffers(vk::CommandBuffer commandBuffer) const
{
    const glm::mat4 sceneViewProj = camera->GetProjectionMatrix() * camera->GetViewMatrix();
    const glm::vec3 cameraPosition = camera->GetDescription().position;

    BufferHelpers::UpdateBuffer(commandBuffer, cameraData.viewProjBuffer,
            ByteView(sceneViewProj), SyncScope::kVertexShaderRead);

    BufferHelpers::UpdateBuffer(commandBuffer, cameraData.cameraPositionBuffer,
            ByteView(cameraPosition), SyncScope::kFragmentShaderRead);

    const glm::mat4 environmentViewProj = camera->GetProjectionMatrix() * glm::mat4(glm::mat3(camera->GetViewMatrix()));

    BufferHelpers::UpdateBuffer(commandBuffer, environmentData.viewProjBuffer,
            ByteView(environmentViewProj), SyncScope::kVertexShaderRead);
}

void RenderSystem::DrawEnvironment(vk::CommandBuffer commandBuffer) const
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, environmentPipeline->Get());

    commandBuffer.bindIndexBuffer(environmentData.indexBuffer, 0, vk::IndexType::eUint16);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            environmentPipeline->GetLayout(), 0, { environmentData.descriptorSet.value }, {});

    const uint32_t indexCount = static_cast<uint32_t>(Details::kEnvironmentIndices.size());

    commandBuffer.drawIndexed(indexCount, 1, 0, 0, 0);
}

void RenderSystem::DrawScene(vk::CommandBuffer commandBuffer) const
{
    const Scene::Hierarchy& sceneHierarchy = scene->GetHierarchy();

    for (const auto& [state, pipeline, materialIndices] : scenePipelines)
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->Get());

        const std::vector<vk::DescriptorSet> descriptorSets{
            cameraData.descriptorSet.value,
            lightingData.descriptorSet.value,
            scene->GetDescriptorSets().rayTracing.value
        };

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                pipeline->GetLayout(), 0, descriptorSets, {});

        for (uint32_t i : materialIndices)
        {
            const uint32_t firstSet = static_cast<uint32_t>(descriptorSets.size());

            const std::vector<vk::DescriptorSet> materialDescriptorSets{
                scene->GetDescriptorSets().materials.values[i]
            };

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                    pipeline->GetLayout(), firstSet, materialDescriptorSets, {});

            for (const auto& renderObject : scene->GetRenderObjects(i))
            {
                const Scene::Mesh& mesh = sceneHierarchy.meshes[renderObject.meshIndex];

                commandBuffer.bindVertexBuffers(0, { mesh.vertexBuffer }, { 0 });
                commandBuffer.bindIndexBuffer(mesh.indexBuffer, 0, mesh.indexType);

                commandBuffer.pushConstants<glm::mat4>(pipeline->GetLayout(),
                        vk::ShaderStageFlagBits::eVertex, 0, { renderObject.transform });

                commandBuffer.drawIndexed(mesh.indexCount, 1, 0, 0, 0);
            }
        }
    }
}

void RenderSystem::HandleResizeEvent(const vk::Extent2D& extent)
{
    if (extent.width != 0 && extent.height != 0)
    {
        for (const auto& framebuffer : framebuffers)
        {
            VulkanContext::device->Get().destroyFramebuffer(framebuffer);
        }

        for (const auto& depthAttachment : depthAttachments)
        {
            VulkanContext::imageManager->DestroyImage(depthAttachment.image);
        }

        forwardRenderPass = Details::CreateForwardRenderPass();

        SetupPipelines();
        SetupDepthAttachments();
        SetupFramebuffers();
    }
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

    SetupPipelines();
}
