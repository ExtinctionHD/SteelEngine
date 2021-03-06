#include "Engine/Systems/RenderSystem.hpp"

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
#include "Shaders/Hybrid/Hybrid.h"

namespace Details
{
    static constexpr std::array<vk::Format, 5> kGBufferFormats{
        vk::Format::eA2B10G10R10UnormPack32, // normals
        vk::Format::eB10G11R11UfloatPack32,  // emission
        vk::Format::eR8G8B8A8Unorm,          // albedo + occlusion
        vk::Format::eR8G8Unorm,              // roughness + metallic
        vk::Format::eD32Sfloat               // depth
    };

    static constexpr vk::Format kDepthFormat = kGBufferFormats.back();

    static constexpr vk::SampleCountFlagBits kSampleCount = vk::SampleCountFlagBits::e1;

    static constexpr uint32_t kGBufferColorAttachmentCount = static_cast<uint32_t>(kGBufferFormats.size() - 1);

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
        std::vector<RenderPass::AttachmentDescription> attachments(kGBufferFormats.size());

        for (size_t i = 0; i < attachments.size(); ++i)
        {
            if (ImageHelpers::IsDepthFormat(kGBufferFormats[i]))
            {
                attachments[i] = RenderPass::AttachmentDescription{
                    RenderPass::AttachmentUsage::eDepth,
                    kGBufferFormats[i],
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    vk::ImageLayout::eShaderReadOnlyOptimal
                };
            }
            else
            {
                attachments[i] = RenderPass::AttachmentDescription{
                    RenderPass::AttachmentUsage::eColor,
                    kGBufferFormats[i],
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    vk::ImageLayout::eGeneral,
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::ImageLayout::eGeneral
                };
            }
        }

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

    static std::unique_ptr<ComputePipeline> CreateLightingPipeline(const Scene& scene,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
    {
        const uint32_t pointLightCount = static_cast<uint32_t>(scene.GetHierarchy().pointLights.size());
        const uint32_t materialCount = static_cast<uint32_t>(scene.GetHierarchy().materials.size());

        const std::tuple specializationValues = std::make_tuple(
                kWorkGroupSize.x, kWorkGroupSize.y, 1, materialCount);

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute, Filepath("~/Shaders/Hybrid/Lighting.comp"),
                { std::make_pair("POINT_LIGHT_COUNT", pointLightCount) }, specializationValues);

        const vk::PushConstantRange pushConstantRange(
                vk::ShaderStageFlagBits::eCompute, 0, sizeof(glm::vec3));

        const ComputePipeline::Description description{
            shaderModule, descriptorSetLayouts, { pushConstantRange }
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(description);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
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

    std::vector<vk::ClearValue> GetGBufferClearValues()
    {
        std::vector<vk::ClearValue> clearValues;
        clearValues.reserve(kGBufferFormats.size());

        for (size_t i = 0; i < kGBufferColorAttachmentCount; ++i)
        {
            clearValues.emplace_back(VulkanHelpers::kDefaultClearColorValue);
        }

        clearValues.emplace_back(VulkanHelpers::kDefaultDepthStencilValue);

        return clearValues;
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
    DescriptorHelpers::DestroyDescriptorSet(gBufferData.descriptorSet);
    VulkanContext::device->Get().destroyFramebuffer(gBufferData.framebuffer);
    for (const auto& renderTarget : gBufferData.textures)
    {
        VulkanContext::imageManager->DestroyImage(renderTarget.image);
    }

    DescriptorHelpers::DestroyMultiDescriptorSet(swapchainData.descriptorSet);
    for (const auto& framebuffer : swapchainData.framebuffers)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }

    DescriptorHelpers::DestroyMultiDescriptorSet(cameraData.descriptorSet);
    for (const auto& buffer : cameraData.cameraBuffers)
    {
        VulkanContext::bufferManager->DestroyBuffer(buffer);
    }

    DescriptorHelpers::DestroyMultiDescriptorSet(lightingData.descriptorSet);
    VulkanContext::bufferManager->DestroyBuffer(lightingData.directLightBuffer);
    for (const auto& buffer : lightingData.cameraBuffers)
    {
        VulkanContext::bufferManager->DestroyBuffer(buffer);
    }

    DescriptorHelpers::DestroyMultiDescriptorSet(environmentData.descriptorSet);
    VulkanContext::bufferManager->DestroyBuffer(environmentData.indexBuffer);
    for (const auto& buffer : environmentData.cameraBuffers)
    {
        VulkanContext::bufferManager->DestroyBuffer(buffer);
    }

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
    UpdateCameraBuffers(commandBuffer, imageIndex);

    ExecuteGBufferRenderPass(commandBuffer, imageIndex);

    ComputeLighting(commandBuffer, imageIndex);

    ExecuteForwardRenderPass(commandBuffer, imageIndex);
}

void RenderSystem::SetupGBufferData()
{
    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    gBufferData.textures.resize(Details::kGBufferFormats.size());

    for (size_t i = 0; i < gBufferData.textures.size(); ++i)
    {
        constexpr vk::ImageUsageFlags colorImageUsage
                = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage;

        constexpr vk::ImageUsageFlags depthImageUsage
                = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;

        const vk::Format format = Details::kGBufferFormats[i];

        const vk::ImageUsageFlags imageUsage = ImageHelpers::IsDepthFormat(format)
                ? depthImageUsage : colorImageUsage;

        gBufferData.textures[i] = ImageHelpers::CreateRenderTarget(
                format, extent, Details::kSampleCount, imageUsage);
    }

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

            for (size_t i = 0; i < gBufferData.textures.size(); ++i)
            {
                const vk::Image image = gBufferData.textures[i].image;

                const vk::Format format = Details::kGBufferFormats[i];

                const vk::ImageSubresourceRange& subresourceRange = ImageHelpers::IsDepthFormat(format)
                        ? ImageHelpers::kFlatDepth : ImageHelpers::kFlatColor;

                const ImageLayoutTransition& layoutTransition = ImageHelpers::IsDepthFormat(format)
                        ? depthLayoutTransition : colorLayoutTransition;

                ImageHelpers::TransitImageLayout(commandBuffer, image, subresourceRange, layoutTransition);
            }
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

    DescriptorSetDescription descriptorSetDescription(gBufferData.textures.size());
    DescriptorSetData descriptorSetData(gBufferData.textures.size());

    for (size_t i = 0; i < gBufferData.textures.size(); ++i)
    {
        const vk::Format format = Details::kGBufferFormats[i];

        const vk::ImageView view = gBufferData.textures[i].view;

        if (ImageHelpers::IsDepthFormat(format))
        {
            descriptorSetDescription[i] = sampledImageDescriptorDescription;
            descriptorSetData[i] = DescriptorHelpers::GetData(Renderer::texelSampler, view);
        }
        else
        {
            descriptorSetDescription[i] = storageImageDescriptorDescription;
            descriptorSetData[i] = DescriptorHelpers::GetData(view);
        }
    }

    gBufferData.descriptorSet = DescriptorHelpers::CreateDescriptorSet(descriptorSetDescription, descriptorSetData);
}

void RenderSystem::SetupSwapchainData()
{
    constexpr vk::ShaderStageFlags shaderStages = vk::ShaderStageFlagBits::eCompute;

    swapchainData.descriptorSet = DescriptorHelpers::CreateSwapchainDescriptorSet(shaderStages);
}

void RenderSystem::SetupCameraData()
{
    const BufferDescription cameraBufferDescription{
        sizeof(glm::mat4),
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    cameraData.cameraBuffers.resize(VulkanContext::swapchain->GetImages().size());

    std::vector<DescriptorSetData> multiDescriptorSetData;
    multiDescriptorSetData.reserve(cameraData.cameraBuffers.size());

    for (auto& buffer : cameraData.cameraBuffers)
    {
        buffer = VulkanContext::bufferManager->CreateBuffer(
                cameraBufferDescription, BufferCreateFlagBits::eStagingBuffer);

        const DescriptorSetData descriptorSetData{
            DescriptorHelpers::GetData(buffer)
        };

        multiDescriptorSetData.push_back(descriptorSetData);
    }

    const DescriptorSetDescription descriptorSetDescription{
        DescriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eVertex,
            vk::DescriptorBindingFlags()
        }
    };

    cameraData.descriptorSet = DescriptorHelpers::CreateMultiDescriptorSet(
            descriptorSetDescription, multiDescriptorSetData);
}

void RenderSystem::SetupLightingData()
{
    const BufferDescription cameraBufferDescription{
        sizeof(glm::mat4),
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    const Texture& irradianceTexture = environment->GetIrradianceTexture();
    const Texture& reflectionTexture = environment->GetReflectionTexture();
    const Texture& specularBRDF = Renderer::imageBasedLighting->GetSpecularBRDF();

    const ImageBasedLighting::Samplers& iblSamplers = Renderer::imageBasedLighting->GetSamplers();

    const DirectLight& directLight = environment->GetDirectLight();

    lightingData.directLightBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eUniformBuffer, ByteView(directLight));

    lightingData.cameraBuffers.resize(VulkanContext::swapchain->GetImages().size());

    std::vector<DescriptorSetData> multiDescriptorSetData;
    multiDescriptorSetData.reserve(lightingData.cameraBuffers.size());

    for (auto& buffer : lightingData.cameraBuffers)
    {
        buffer = VulkanContext::bufferManager->CreateBuffer(
                cameraBufferDescription, BufferCreateFlagBits::eStagingBuffer);

        const DescriptorSetData descriptorSetData{
            DescriptorHelpers::GetData(buffer),
            DescriptorHelpers::GetData(iblSamplers.irradiance, irradianceTexture.view),
            DescriptorHelpers::GetData(iblSamplers.reflection, reflectionTexture.view),
            DescriptorHelpers::GetData(iblSamplers.specularBRDF, specularBRDF.view),
            DescriptorHelpers::GetData(lightingData.directLightBuffer),
        };

        multiDescriptorSetData.push_back(descriptorSetData);
    }

    const DescriptorSetDescription descriptorSetDescription{
        DescriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        },
    };

    lightingData.descriptorSet = DescriptorHelpers::CreateMultiDescriptorSet(
            descriptorSetDescription, multiDescriptorSetData);
}

void RenderSystem::SetupEnvironmentData()
{
    environmentData.indexBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eIndexBuffer, ByteView(Details::kEnvironmentIndices));

    const BufferDescription cameraBufferDescription{
        sizeof(glm::mat4),
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    environmentData.cameraBuffers.resize(VulkanContext::swapchain->GetImages().size());

    std::vector<DescriptorSetData> multiDescriptorSetData;
    multiDescriptorSetData.reserve(environmentData.cameraBuffers.size());

    for (auto& buffer : environmentData.cameraBuffers)
    {
        buffer = VulkanContext::bufferManager->CreateBuffer(
                cameraBufferDescription, BufferCreateFlagBits::eStagingBuffer);

        const DescriptorSetData descriptorSetData{
            DescriptorHelpers::GetData(buffer),
            DescriptorHelpers::GetData(Renderer::defaultSampler, environment->GetTexture().view)
        };

        multiDescriptorSetData.push_back(descriptorSetData);
    }

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

    environmentData.descriptorSet = DescriptorHelpers::CreateMultiDescriptorSet(
            descriptorSetDescription, multiDescriptorSetData);
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

    std::vector<vk::ImageView> gBufferImageViews(gBufferData.textures.size());
    vk::ImageView depthImageView;

    for (size_t i = 0; i < gBufferData.textures.size(); ++i)
    {
        gBufferImageViews[i] = gBufferData.textures[i].view;

        const vk::Format format = Details::kGBufferFormats[i];

        if (ImageHelpers::IsDepthFormat(format))
        {
            depthImageView = gBufferImageViews[i];
        }
    }

    const std::vector<vk::ImageView>& swapchainImageViews = VulkanContext::swapchain->GetImageViews();

    gBufferData.framebuffer = VulkanHelpers::CreateFramebuffers(device,
            gBufferRenderPass->Get(), extent, {}, gBufferImageViews).front();

    swapchainData.framebuffers = VulkanHelpers::CreateFramebuffers(device,
            forwardRenderPass->Get(), extent, { swapchainImageViews }, { depthImageView });
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

    std::vector<vk::DescriptorSetLayout> lightingPipelineLayouts{
        swapchainData.descriptorSet.layout,
        gBufferData.descriptorSet.layout,
        lightingData.descriptorSet.layout,
        sceneDescriptorSets.rayTracing.layout
    };

    if (sceneDescriptorSets.pointLights.has_value())
    {
        lightingPipelineLayouts.push_back(sceneDescriptorSets.pointLights.value().layout);
    }

    lightingPipeline = Details::CreateLightingPipeline(*scene, lightingPipelineLayouts);

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

void RenderSystem::UpdateCameraBuffers(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const glm::mat4& view = camera->GetViewMatrix();
    const glm::mat4& proj = camera->GetProjectionMatrix();

    const glm::mat4 viewProj = proj * view;
    BufferHelpers::UpdateBuffer(commandBuffer, cameraData.cameraBuffers[imageIndex],
            ByteView(viewProj), SyncScope::kWaitForNone, SyncScope::kVertexUniformRead);

    const glm::mat4 inverseProjView = glm::inverse(view) * glm::inverse(proj);
    BufferHelpers::UpdateBuffer(commandBuffer, lightingData.cameraBuffers[imageIndex],
            ByteView(inverseProjView), SyncScope::kWaitForNone, SyncScope::kComputeShaderRead);

    const glm::mat4 environmentViewProj = proj * glm::mat4(glm::mat3(view));
    BufferHelpers::UpdateBuffer(commandBuffer, environmentData.cameraBuffers[imageIndex],
            ByteView(environmentViewProj), SyncScope::kWaitForNone, SyncScope::kVertexUniformRead);
}

void RenderSystem::ExecuteGBufferRenderPass(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    const vk::Rect2D renderArea(vk::Offset2D(0, 0), extent);

    const std::vector<vk::ClearValue> clearValues = Details::GetGBufferClearValues();

    const vk::RenderPassBeginInfo beginInfo(
            gBufferRenderPass->Get(),
            gBufferData.framebuffer,
            renderArea, clearValues);

    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

    DrawScene(commandBuffer, imageIndex);

    commandBuffer.endRenderPass();
}

void RenderSystem::ComputeLighting(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];

    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    const glm::vec3& cameraPosition = camera->GetDescription().position;

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

    const Scene::DescriptorSets& sceneDescriptorSets = scene->GetDescriptorSets();

    std::vector<vk::DescriptorSet> descriptorSets{
        swapchainData.descriptorSet.values[imageIndex],
        gBufferData.descriptorSet.value,
        lightingData.descriptorSet.values[imageIndex],
        sceneDescriptorSets.rayTracing.value
    };

    if (sceneDescriptorSets.pointLights.has_value())
    {
        descriptorSets.push_back(sceneDescriptorSets.pointLights.value().value);
    }

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, lightingPipeline->Get());

    commandBuffer.pushConstants<glm::vec3>(lightingPipeline->GetLayout(),
            vk::ShaderStageFlagBits::eCompute, 0, { cameraPosition });

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
        DrawPointLights(commandBuffer, imageIndex);
    }

    DrawEnvironment(commandBuffer, imageIndex);

    commandBuffer.endRenderPass();
}

void RenderSystem::DrawScene(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const glm::vec3& cameraPosition = camera->GetDescription().position;
    const Scene::Hierarchy& sceneHierarchy = scene->GetHierarchy();
    const Scene::DescriptorSets& sceneDescriptorSets = scene->GetDescriptorSets();

    for (const auto& [state, pipeline, materialIndices] : scenePipelines)
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->Get());

        commandBuffer.pushConstants<glm::vec3>(pipeline->GetLayout(),
                vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4), { cameraPosition });

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                pipeline->GetLayout(), 0, { cameraData.descriptorSet.values[imageIndex] }, {});

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

void RenderSystem::DrawEnvironment(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, environmentPipeline->Get());

    commandBuffer.bindIndexBuffer(environmentData.indexBuffer, 0, vk::IndexType::eUint16);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            environmentPipeline->GetLayout(), 0, { environmentData.descriptorSet.values[imageIndex] }, {});

    const uint32_t indexCount = static_cast<uint32_t>(Details::kEnvironmentIndices.size());

    commandBuffer.drawIndexed(indexCount, 1, 0, 0, 0);
}

void RenderSystem::DrawPointLights(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const std::vector<vk::Buffer> vertexBuffers{
        pointLightsData.vertexBuffer,
        pointLightsData.instanceBuffer
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pointLightsPipeline->Get());

    commandBuffer.bindIndexBuffer(pointLightsData.indexBuffer, 0, vk::IndexType::eUint32);
    commandBuffer.bindVertexBuffers(0, vertexBuffers, { 0, 0 });

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            pointLightsPipeline->GetLayout(), 0, { cameraData.descriptorSet.values[imageIndex] }, {});

    commandBuffer.drawIndexed(pointLightsData.indexCount, pointLightsData.instanceCount, 0, 0, 0);
}

void RenderSystem::HandleResizeEvent(const vk::Extent2D& extent)
{
    if (extent.width != 0 && extent.height != 0)
    {
        DescriptorHelpers::DestroyDescriptorSet(gBufferData.descriptorSet);
        VulkanContext::device->Get().destroyFramebuffer(gBufferData.framebuffer);
        for (const auto& renderTarget : gBufferData.textures)
        {
            VulkanContext::imageManager->DestroyImage(renderTarget.image);
        }

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
