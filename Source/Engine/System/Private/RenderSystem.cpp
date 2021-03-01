#include "Engine/System/RenderSystem.hpp"

#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Scene/MeshHelpers.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Config.hpp"
#include "Engine/Engine.hpp"
#include "Engine/EngineHelpers.hpp"
#include "Engine/InputHelpers.hpp"
#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/Vulkan/ComputeHelpers.hpp"

namespace Details
{
    static constexpr vk::Format kNormalsFormat = vk::Format::eR16G16B16A16Sfloat;
    static constexpr vk::Format kBaseColorFormat = vk::Format::eR8G8B8A8Unorm;
    static constexpr vk::Format kEmissionFormat = vk::Format::eR8G8B8A8Unorm;
    static constexpr vk::Format kMiscFormat = vk::Format::eR8G8B8A8Unorm;
    static constexpr vk::Format kDepthFormat = vk::Format::eD32Sfloat;

    static constexpr vk::SampleCountFlagBits kSampleCount = vk::SampleCountFlagBits::e1;

    static constexpr uint32_t kGBufferColorAttachmentCount = 4;
    static constexpr uint32_t kGBufferSize = kGBufferColorAttachmentCount + 1;

    static constexpr glm::uvec2 kWorkGroupSize(8, 8);

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

    static std::unique_ptr<RenderPass> CreateGBufferRenderPass()
    {
        const std::vector<RenderPass::AttachmentDescription> attachments{
            RenderPass::AttachmentDescription{
                RenderPass::AttachmentUsage::eColor,
                kNormalsFormat,
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eGeneral
            },
            RenderPass::AttachmentDescription{
                RenderPass::AttachmentUsage::eColor,
                kBaseColorFormat,
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eGeneral
            },
            RenderPass::AttachmentDescription{
                RenderPass::AttachmentUsage::eColor,
                kEmissionFormat,
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eGeneral
            },
            RenderPass::AttachmentDescription{
                RenderPass::AttachmentUsage::eColor,
                kMiscFormat,
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eGeneral
            },
            RenderPass::AttachmentDescription{
                RenderPass::AttachmentUsage::eDepth,
                kDepthFormat,
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                vk::ImageLayout::eDepthStencilAttachmentOptimal,
                vk::ImageLayout::eDepthStencilAttachmentOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal
            }
        };

        const RenderPass::Description description{
            vk::PipelineBindPoint::eGraphics,
            kSampleCount, attachments
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

    static std::unique_ptr<RenderPass> CreateForwardRenderPass()
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
                kDepthFormat,
                vk::AttachmentLoadOp::eLoad,
                vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eDepthStencilAttachmentOptimal,
                vk::ImageLayout::eDepthStencilAttachmentOptimal
            }
        };

        const RenderPass::Description description{
            vk::PipelineBindPoint::eGraphics,
            kSampleCount, attachments
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

    static std::unique_ptr<GraphicsPipeline> CreateGBufferPipeline(const RenderPass& renderPass,
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
                    Filepath("~/Shaders/Hybrid/GBuffer.vert"), {}),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eFragment,
                    Filepath("~/Shaders/Hybrid/GBuffer.frag"), defines)
        };

        const VertexDescription vertexDescription{
            Scene::Mesh::Vertex::kFormat,
            vk::VertexInputRate::eVertex
        };

        const std::vector<BlendMode> blendModes = Repeat(BlendMode::eDisabled, kGBufferColorAttachmentCount);

        const std::vector<vk::PushConstantRange> pushConstantRanges{
            vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4)),
            vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4), sizeof(glm::vec3))
        };

        const GraphicsPipeline::Description description{
            VulkanContext::swapchain->GetExtent(),
            vk::PrimitiveTopology::eTriangleList,
            vk::PolygonMode::eFill,
            cullMode,
            vk::FrontFace::eCounterClockwise,
            kSampleCount,
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

    static std::unique_ptr<ComputePipeline> CreateLightingPipeline(
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
    {
        const std::tuple specializationValues = std::make_tuple(
                kWorkGroupSize.x, kWorkGroupSize.y, 1);

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute,
                Filepath("~/Shaders/Hybrid/Lighting.comp"),
                {}, specializationValues);

        const ComputePipeline::Description description{
            shaderModule, descriptorSetLayouts, {}
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(description);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

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
            kSampleCount,
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
            VulkanContext::swapchain->GetExtent(),
            vk::PrimitiveTopology::eTriangleList,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            vk::FrontFace::eCounterClockwise,
            kSampleCount,
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
}

RenderSystem::RenderSystem(Scene* scene_, Camera* camera_, Environment* environment_)
    : scene(scene_)
    , camera(camera_)
    , environment(environment_)
{
    SetupCameraData();
    SetupLightingData();
    SetupEnvironmentData();
    SetupPointLightsData();

    SetupGBufferData();
    SetupSwapchainData();

    SetupRenderPasses();
    SetupFramebuffers();
    SetupPipelines();

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &RenderSystem::HandleResizeEvent));

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &RenderSystem::HandleKeyInputEvent));
}

RenderSystem::~RenderSystem()
{
    VulkanContext::imageManager->DestroyImage(gBufferData.normals.image);
    VulkanContext::imageManager->DestroyImage(gBufferData.baseColor.image);
    VulkanContext::imageManager->DestroyImage(gBufferData.emission.image);
    VulkanContext::imageManager->DestroyImage(gBufferData.misc.image);
    VulkanContext::imageManager->DestroyImage(gBufferData.depth.image);

    DescriptorHelpers::DestroyDescriptorSet(gBufferData.descriptorSet);
    VulkanContext::device->Get().destroyFramebuffer(gBufferData.framebuffer);

    DescriptorHelpers::DestroyMultiDescriptorSet(swapchainData.descriptorSet);
    for (const auto& framebuffer : swapchainData.framebuffers)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }

    VulkanContext::bufferManager->DestroyBuffer(cameraData.viewProjBuffer);
    DescriptorHelpers::DestroyDescriptorSet(cameraData.descriptorSet);

    VulkanContext::bufferManager->DestroyBuffer(lightingData.directLightBuffer);
    DescriptorHelpers::DestroyDescriptorSet(lightingData.descriptorSet);

    VulkanContext::bufferManager->DestroyBuffer(environmentData.indexBuffer);
    VulkanContext::bufferManager->DestroyBuffer(environmentData.viewProjBuffer);
    DescriptorHelpers::DestroyDescriptorSet(environmentData.descriptorSet);

    if (pointLightsData.instanceCount > 0)
    {
        VulkanContext::bufferManager->DestroyBuffer(pointLightsData.indexBuffer);
        VulkanContext::bufferManager->DestroyBuffer(pointLightsData.vertexBuffer);
        VulkanContext::bufferManager->DestroyBuffer(pointLightsData.instanceBuffer);
    }
}

void RenderSystem::Process(float) {}

void RenderSystem::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    UpdateCameraBuffers(commandBuffer);

    ExecuteGBufferRenderPass(commandBuffer);

    ComputeLighting(commandBuffer, imageIndex);

    ExecuteForwardRenderPass(commandBuffer, imageIndex);
}

void RenderSystem::SetupGBufferData()
{
    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    auto& [normals, baseColor, emission, misc, depth, descriptorSet, framebuffer] = gBufferData;

    normals = ImageHelpers::CreateRenderTarget(Details::kNormalsFormat, extent, Details::kSampleCount,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage);

    baseColor = ImageHelpers::CreateRenderTarget(Details::kBaseColorFormat, extent, Details::kSampleCount,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage);

    emission = ImageHelpers::CreateRenderTarget(Details::kEmissionFormat, extent, Details::kSampleCount,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage);

    misc = ImageHelpers::CreateRenderTarget(Details::kMiscFormat, extent, Details::kSampleCount,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage);

    depth = ImageHelpers::CreateRenderTarget(Details::kDepthFormat, extent, Details::kSampleCount,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);

    VulkanContext::device->ExecuteOneTimeCommands([this](vk::CommandBuffer commandBuffer)
        {
            const ImageLayoutTransition colorLayoutTransition{
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eGeneral,
                PipelineBarrier{
                    SyncScope::kWaitForNone,
                    SyncScope::kBlockNone
                }
            };

            const ImageLayoutTransition depthLayoutTransition{
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eDepthStencilAttachmentOptimal,
                PipelineBarrier{
                    SyncScope::kWaitForNone,
                    SyncScope::kBlockNone
                }
            };

            ImageHelpers::TransitImageLayout(commandBuffer, gBufferData.normals.image,
                    ImageHelpers::kFlatColor, colorLayoutTransition);
            ImageHelpers::TransitImageLayout(commandBuffer, gBufferData.baseColor.image,
                    ImageHelpers::kFlatColor, colorLayoutTransition);
            ImageHelpers::TransitImageLayout(commandBuffer, gBufferData.emission.image,
                    ImageHelpers::kFlatColor, colorLayoutTransition);
            ImageHelpers::TransitImageLayout(commandBuffer, gBufferData.misc.image,
                    ImageHelpers::kFlatColor, colorLayoutTransition);
            ImageHelpers::TransitImageLayout(commandBuffer, gBufferData.depth.image,
                    ImageHelpers::kFlatDepth, depthLayoutTransition);
        });

    const DescriptorDescription storageImageDescriptorDescription{
        1, vk::DescriptorType::eStorageImage,
        vk::ShaderStageFlagBits::eCompute,
        vk::DescriptorBindingFlags()
    };

    const DescriptorDescription sampledImageDescriptorDescription{
        1, vk::DescriptorType::eCombinedImageSampler,
        vk::ShaderStageFlagBits::eCompute,
        vk::DescriptorBindingFlags()
    };

    const DescriptorSetDescription descriptorSetDescription{
        storageImageDescriptorDescription,
        storageImageDescriptorDescription,
        storageImageDescriptorDescription,
        storageImageDescriptorDescription,
        sampledImageDescriptorDescription
    };

    const DescriptorSetData descriptorSetData{
        DescriptorHelpers::GetData(normals.view),
        DescriptorHelpers::GetData(baseColor.view),
        DescriptorHelpers::GetData(emission.view),
        DescriptorHelpers::GetData(misc.view),
        DescriptorHelpers::GetData(Renderer::texelSampler, depth.view)
    };

    descriptorSet = DescriptorHelpers::CreateDescriptorSet(descriptorSetDescription, descriptorSetData);
}

void RenderSystem::SetupSwapchainData()
{
    constexpr vk::ShaderStageFlags shaderStages = vk::ShaderStageFlagBits::eCompute;

    swapchainData.descriptorSet = DescriptorHelpers::CreateSwapchainDescriptorSet(shaderStages);
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

    const DescriptorSetDescription descriptorSetDescription{
        DescriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eVertex,
            vk::DescriptorBindingFlags()
        }
    };

    const DescriptorSetData descriptorSetData{
        DescriptorHelpers::GetData(cameraData.viewProjBuffer)
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

    lightingData.directLightBuffer = BufferHelpers::CreateBufferWithData(
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
        DescriptorHelpers::GetData(lightingData.directLightBuffer),
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

void RenderSystem::SetupPointLightsData()
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

void RenderSystem::SetupRenderPasses()
{
    gBufferRenderPass = Details::CreateGBufferRenderPass();
    forwardRenderPass = Details::CreateForwardRenderPass();
}

void RenderSystem::SetupFramebuffers()
{
    const vk::Device device = VulkanContext::device->Get();
    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    const std::vector<vk::ImageView> gBufferImageViews{
        gBufferData.normals.view,
        gBufferData.baseColor.view,
        gBufferData.emission.view,
        gBufferData.misc.view,
        gBufferData.depth.view
    };

    const std::vector<vk::ImageView>& swapchainImageViews = VulkanContext::swapchain->GetImageViews();

    gBufferData.framebuffer = VulkanHelpers::CreateFramebuffers(device,
            gBufferRenderPass->Get(), extent, {}, gBufferImageViews).front();

    swapchainData.framebuffers = VulkanHelpers::CreateFramebuffers(device,
            forwardRenderPass->Get(), extent, { swapchainImageViews }, { gBufferData.depth.view });
}

void RenderSystem::SetupPipelines()
{
    scenePipelines.clear();

    const Scene::Hierarchy& sceneHierarchy = scene->GetHierarchy();
    const Scene::DescriptorSets& sceneDescriptorSets = scene->GetDescriptorSets();

    const std::vector<vk::DescriptorSetLayout> scenePipelineLayouts{
        cameraData.descriptorSet.layout,
        sceneDescriptorSets.materials.layout
    };

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
            std::unique_ptr<GraphicsPipeline> pipeline = Details::CreateGBufferPipeline(
                    *gBufferRenderPass, scenePipelineLayouts, material.pipelineState);

            scenePipelines.push_back(ScenePipeline{ material.pipelineState, std::move(pipeline), { i } });
        }
    }

    const std::vector<vk::DescriptorSetLayout> lightingPipelineLayouts{
        swapchainData.descriptorSet.layout,
        gBufferData.descriptorSet.layout
    };

    lightingPipeline = Details::CreateLightingPipeline(lightingPipelineLayouts);

    const std::vector<vk::DescriptorSetLayout> environmentPipelineLayouts{
        environmentData.descriptorSet.layout
    };

    environmentPipeline = Details::CreateEnvironmentPipeline(*forwardRenderPass, environmentPipelineLayouts);

    if (pointLightsData.instanceCount > 0)
    {
        const std::vector<vk::DescriptorSetLayout> pointLightsPipelineLayouts{
            cameraData.descriptorSet.layout
        };

        pointLightsPipeline = Details::CreatePointLightsPipeline(*forwardRenderPass, pointLightsPipelineLayouts);
    }
}

void RenderSystem::UpdateCameraBuffers(vk::CommandBuffer commandBuffer) const
{
    const glm::mat4 sceneViewProj = camera->GetProjectionMatrix() * camera->GetViewMatrix();
    const glm::mat4 environmentViewProj = camera->GetProjectionMatrix() * glm::mat4(glm::mat3(camera->GetViewMatrix()));

    BufferHelpers::UpdateBuffer(commandBuffer, cameraData.viewProjBuffer,
            ByteView(sceneViewProj), SyncScope::kVertexUniformRead, SyncScope::kVertexUniformRead);

    BufferHelpers::UpdateBuffer(commandBuffer, environmentData.viewProjBuffer,
            ByteView(environmentViewProj), SyncScope::kVertexUniformRead, SyncScope::kVertexUniformRead);
}

void RenderSystem::ExecuteGBufferRenderPass(vk::CommandBuffer commandBuffer) const
{
    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    const vk::Rect2D renderArea(vk::Offset2D(0, 0), extent);

    const std::vector<vk::ClearValue> clearValues{
        VulkanHelpers::kDefaultClearColorValue,
        VulkanHelpers::kDefaultClearColorValue,
        VulkanHelpers::kDefaultClearColorValue,
        VulkanHelpers::kDefaultClearColorValue,
        VulkanHelpers::kDefaultDepthStencilValue
    };

    const vk::RenderPassBeginInfo beginInfo(
            gBufferRenderPass->Get(),
            gBufferData.framebuffer,
            renderArea, clearValues);

    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

    DrawScene(commandBuffer);

    commandBuffer.endRenderPass();
}

void RenderSystem::ComputeLighting(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];

    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    const ImageLayoutTransition layoutTransition{
        vk::ImageLayout::ePresentSrcKHR,
        vk::ImageLayout::eGeneral,
        PipelineBarrier{
            SyncScope::kWaitForNone,
            SyncScope::kComputeShaderWrite
        }
    };

    ImageHelpers::TransitImageLayout(commandBuffer, swapchainImage,
            ImageHelpers::kFlatColor, layoutTransition);

    const std::vector<vk::DescriptorSet> descriptorSets{
        swapchainData.descriptorSet.values[imageIndex],
        gBufferData.descriptorSet.value
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, lightingPipeline->Get());

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
            lightingPipeline->GetLayout(), 0, descriptorSets, {});

    const glm::uvec3 groupCount = ComputeHelpers::CalculateWorkGroupCount(extent, Details::kWorkGroupSize);

    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
}

void RenderSystem::ExecuteForwardRenderPass(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    const vk::Rect2D renderArea(vk::Offset2D(0, 0), extent);

    const vk::RenderPassBeginInfo beginInfo(
            forwardRenderPass->Get(),
            swapchainData.framebuffers[imageIndex],
            renderArea);

    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

    if (pointLightsData.instanceCount > 0)
    {
        DrawPointLights(commandBuffer);
    }

    DrawEnvironment(commandBuffer);

    commandBuffer.endRenderPass();
}

void RenderSystem::DrawScene(vk::CommandBuffer commandBuffer) const
{
    const glm::vec3& cameraPosition = camera->GetDescription().position;
    const Scene::Hierarchy& sceneHierarchy = scene->GetHierarchy();

    for (const auto& [state, pipeline, materialIndices] : scenePipelines)
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->Get());

        const Scene::DescriptorSets& sceneDescriptorSets = scene->GetDescriptorSets();

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                pipeline->GetLayout(), 0, { cameraData.descriptorSet.value }, {});

        commandBuffer.pushConstants<glm::vec3>(pipeline->GetLayout(),
                vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4), { cameraPosition });

        for (uint32_t i : materialIndices)
        {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                    pipeline->GetLayout(), 1, { sceneDescriptorSets.materials.values[i] }, {});

            for (const auto& renderObject : scene->GetRenderObjects(i))
            {
                const Scene::Mesh& mesh = sceneHierarchy.meshes[renderObject.meshIndex];

                commandBuffer.bindIndexBuffer(mesh.indexBuffer, 0, mesh.indexType);
                commandBuffer.bindVertexBuffers(0, { mesh.vertexBuffer }, { 0 });

                commandBuffer.pushConstants<glm::mat4>(pipeline->GetLayout(),
                        vk::ShaderStageFlagBits::eVertex, 0, { renderObject.transform });

                commandBuffer.drawIndexed(mesh.indexCount, 1, 0, 0, 0);
            }
        }
    }
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

void RenderSystem::DrawPointLights(vk::CommandBuffer commandBuffer) const
{
    const std::vector<vk::Buffer> vertexBuffers{
        pointLightsData.vertexBuffer,
        pointLightsData.instanceBuffer
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pointLightsPipeline->Get());

    commandBuffer.bindIndexBuffer(pointLightsData.indexBuffer, 0, vk::IndexType::eUint32);
    commandBuffer.bindVertexBuffers(0, vertexBuffers, { 0, 0 });

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            pointLightsPipeline->GetLayout(), 0, { cameraData.descriptorSet.value }, {});

    commandBuffer.drawIndexed(pointLightsData.indexCount, pointLightsData.instanceCount, 0, 0, 0);
}

void RenderSystem::HandleResizeEvent(const vk::Extent2D& extent)
{
    if (extent.width != 0 && extent.height != 0)
    {
        VulkanContext::imageManager->DestroyImage(gBufferData.normals.image);
        VulkanContext::imageManager->DestroyImage(gBufferData.baseColor.image);
        VulkanContext::imageManager->DestroyImage(gBufferData.emission.image);
        VulkanContext::imageManager->DestroyImage(gBufferData.misc.image);
        VulkanContext::imageManager->DestroyImage(gBufferData.depth.image);

        DescriptorHelpers::DestroyDescriptorSet(gBufferData.descriptorSet);
        VulkanContext::device->Get().destroyFramebuffer(gBufferData.framebuffer);

        DescriptorHelpers::DestroyMultiDescriptorSet(swapchainData.descriptorSet);
        for (const auto& framebuffer : swapchainData.framebuffers)
        {
            VulkanContext::device->Get().destroyFramebuffer(framebuffer);
        }

        SetupGBufferData();
        SetupSwapchainData();

        SetupRenderPasses();
        SetupFramebuffers();
        SetupPipelines();
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
