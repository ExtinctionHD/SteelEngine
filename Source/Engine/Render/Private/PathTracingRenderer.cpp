#include "Engine/Render/PathTracingRenderer.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"
#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Scene/StorageComponents.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Scene/Components.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Config.hpp"
#include "Engine/Engine.hpp"

#include "Shaders/Common/Common.h"

namespace Details
{
    static constexpr uint32_t kDefaultSampleCount = 1;

    static Texture CreateAccumulationTexture(const vk::Extent2D& extent)
    {
        const Texture texture = ImageHelpers::CreateRenderTarget(vk::Format::eR32G32B32A32Sfloat,
                extent, vk::SampleCountFlagBits::e1, vk::ImageUsageFlagBits::eStorage);

        VulkanContext::device->ExecuteOneTimeCommands([&texture](vk::CommandBuffer commandBuffer)
            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eGeneral,
                    PipelineBarrier::kEmpty
                };

                ImageHelpers::TransitImageLayout(commandBuffer, texture.image,
                        ImageHelpers::kFlatColor, layoutTransition);
            });

        return texture;
    }

    static MultiDescriptorSet CreateRenderTargetsDescriptorSet(
            vk::ImageView accumulationView, bool useSwapchainRenderTarget)
    {
        DescriptorSetDescription descriptorSetDescription{
            DescriptorDescription{
                1, vk::DescriptorType::eStorageImage,
                vk::ShaderStageFlagBits::eRaygenKHR,
                vk::DescriptorBindingFlags()
            }
        };

        if (accumulationView)
        {
            const DescriptorDescription descriptorDescription{
                1, vk::DescriptorType::eStorageImage,
                vk::ShaderStageFlagBits::eRaygenKHR,
                vk::DescriptorBindingFlags()
            };

            descriptorSetDescription.push_back(descriptorDescription);
        }

        if (useSwapchainRenderTarget)
        {
            const std::vector<vk::ImageView>& swapchainImageViews = VulkanContext::swapchain->GetImageViews();

            std::vector<DescriptorSetData> multiDescriptorSetData;
            multiDescriptorSetData.reserve(swapchainImageViews.size());

            for (const auto& swapchainImageView : swapchainImageViews)
            {
                multiDescriptorSetData.push_back({ DescriptorHelpers::GetStorageData(swapchainImageView) });
            }

            if (accumulationView)
            {
                const DescriptorData descriptorData
                        = DescriptorHelpers::GetStorageData(accumulationView);

                for (auto& descriptorSetData : multiDescriptorSetData)
                {
                    descriptorSetData.push_back(descriptorData);
                }
            }

            return DescriptorHelpers::CreateMultiDescriptorSet(descriptorSetDescription, multiDescriptorSetData);
        }

        const vk::DescriptorSetLayout descriptorSetLayout
                = VulkanContext::descriptorPool->CreateDescriptorSetLayout(descriptorSetDescription);

        return MultiDescriptorSet{ descriptorSetLayout, {} };
    }

    static CameraData CreateCameraData(uint32_t bufferCount)
    {
        constexpr vk::DeviceSize bufferSize = sizeof(gpu::CameraPT);

        constexpr vk::ShaderStageFlags shaderStages = vk::ShaderStageFlagBits::eRaygenKHR;

        return RenderHelpers::CreateCameraData(bufferCount, bufferSize, shaderStages);
    }

    static DescriptorSet CreateSceneDescriptorSet(const Scene& scene)
    {
        const auto& environmentComponent = scene.ctx().get<EnvironmentComponent>();
        const auto& renderComponent = scene.ctx().get<RenderStorageComponent>();
        const auto& textureComponent = scene.ctx().get<TextureStorageComponent>();
        const auto& geometryComponent = scene.ctx().get<GeometryStorageComponent>();

        const Texture& cubemapTexture = environmentComponent.cubemapTexture;

        const uint32_t textureCount = static_cast<uint32_t>(textureComponent.textures.size());
        const uint32_t primitiveCount = static_cast<uint32_t>(geometryComponent.primitives.size());

        constexpr vk::ShaderStageFlags materialShaderStages = vk::ShaderStageFlagBits::eRaygenKHR
                | vk::ShaderStageFlagBits::eAnyHitKHR;

        constexpr vk::ShaderStageFlags primitiveShaderStages = vk::ShaderStageFlagBits::eRaygenKHR
                | vk::ShaderStageFlagBits::eAnyHitKHR | vk::ShaderStageFlagBits::eClosestHitKHR;

        const DescriptorSetDescription descriptorSetDescription{
            DescriptorDescription{
                1, vk::DescriptorType::eUniformBuffer,
                vk::ShaderStageFlagBits::eRaygenKHR,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                1, vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eRaygenKHR,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                1, vk::DescriptorType::eAccelerationStructureKHR,
                vk::ShaderStageFlagBits::eRaygenKHR,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                1, vk::DescriptorType::eUniformBuffer,
                materialShaderStages,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                textureCount, vk::DescriptorType::eCombinedImageSampler,
                materialShaderStages,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                primitiveCount, vk::DescriptorType::eStorageBuffer,
                primitiveShaderStages,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                primitiveCount, vk::DescriptorType::eStorageBuffer,
                primitiveShaderStages,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                primitiveCount, vk::DescriptorType::eStorageBuffer,
                primitiveShaderStages,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                primitiveCount, vk::DescriptorType::eStorageBuffer,
                primitiveShaderStages,
                vk::DescriptorBindingFlags()
            }
        };
        
        std::vector<vk::Buffer> indexBuffers;
        std::vector<vk::Buffer> normalsBuffers;
        std::vector<vk::Buffer> tangentsBuffers;
        std::vector<vk::Buffer> texCoordBuffers;

        indexBuffers.reserve(geometryComponent.primitives.size());
        normalsBuffers.reserve(geometryComponent.primitives.size());
        tangentsBuffers.reserve(geometryComponent.primitives.size());
        texCoordBuffers.reserve(geometryComponent.primitives.size());

        for (const auto& primitive : geometryComponent.primitives)
        {
            indexBuffers.push_back(primitive.indexBuffer);
            normalsBuffers.push_back(primitive.normalBuffer);
            tangentsBuffers.push_back(primitive.tangentBuffer);
            texCoordBuffers.push_back(primitive.texCoordBuffer);
        }

        const DescriptorSetData descriptorSetData{
            DescriptorHelpers::GetData(renderComponent.lightBuffer),
            DescriptorHelpers::GetData(RenderContext::defaultSampler, cubemapTexture.view),
            DescriptorHelpers::GetData(renderComponent.tlas),
            DescriptorHelpers::GetData(renderComponent.materialBuffer),
            DescriptorHelpers::GetData(textureComponent.textures),
            DescriptorHelpers::GetStorageData(indexBuffers),
            DescriptorHelpers::GetStorageData(normalsBuffers),
            DescriptorHelpers::GetStorageData(tangentsBuffers),
            DescriptorHelpers::GetStorageData(texCoordBuffers),
        };

        return DescriptorHelpers::CreateDescriptorSet(descriptorSetDescription, descriptorSetData);
    }

    static std::unique_ptr<RayTracingPipeline> CreateRayTracingPipeline(const Scene& scene,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts,
            bool accumulation, bool isProbeRenderer, uint32_t sampleCount)
    {
        const auto& materialComponent = scene.ctx().get<MaterialStorageComponent>();

        const uint32_t materialCount = static_cast<uint32_t>(materialComponent.materials.size());

        const uint32_t lightCount = static_cast<uint32_t>(scene.view<LightComponent>().size());

        const ShaderDefines rayGenDefines{
            std::make_pair("ACCUMULATION", accumulation),
            std::make_pair("RENDER_TO_HDR", isProbeRenderer),
            std::make_pair("RENDER_TO_CUBE", isProbeRenderer),
            std::make_pair("BACKFACE_CULLING", !isProbeRenderer),
            std::make_pair("LIGHT_COUNT", lightCount)
        };

        const std::tuple rayGenSpecializationValues = std::make_tuple(
                sampleCount, materialCount, Config::kPointLightRadius);

        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eRaygenKHR,
                    Filepath("~/Shaders/PathTracing/RayGen.rgen"),
                    rayGenDefines, rayGenSpecializationValues),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eMissKHR,
                    Filepath("~/Shaders/PathTracing/Miss.rmiss"),
                    { std::make_pair("PAYLOAD_LOCATION", 0) }),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eClosestHitKHR,
                    Filepath("~/Shaders/PathTracing/ClosestHit.rchit"), {}),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eAnyHitKHR,
                    Filepath("~/Shaders/PathTracing/AnyHit.rahit"), {},
                    std::make_tuple(materialCount))
        };

        std::map<ShaderGroupType, std::vector<ShaderGroup>> shaderGroupsMap;
        shaderGroupsMap[ShaderGroupType::eRaygen] = {
            ShaderGroup{ 0, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR }
        };
        shaderGroupsMap[ShaderGroupType::eMiss] = {
            ShaderGroup{ 1, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR }
        };
        shaderGroupsMap[ShaderGroupType::eHit] = {
            ShaderGroup{ VK_SHADER_UNUSED_KHR, 2, 3, VK_SHADER_UNUSED_KHR }
        };

        const std::vector<vk::PushConstantRange> pushConstantRanges{
            vk::PushConstantRange(vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(uint32_t))
        };

        const RayTracingPipeline::Description description{
            shaderModules, shaderGroupsMap, descriptorSetLayouts,
            accumulation ? pushConstantRanges : std::vector<vk::PushConstantRange>{}
        };

        std::unique_ptr<RayTracingPipeline> pipeline = RayTracingPipeline::Create(description);

        for (const auto& shaderModule : shaderModules)
        {
            VulkanContext::shaderManager->DestroyShaderModule(shaderModule);
        }

        return pipeline;
    }
}

PathTracingRenderer::PathTracingRenderer()
    : isProbeRenderer(false)
    , sampleCount(Details::kDefaultSampleCount)
{
    EASY_FUNCTION()

    renderTargets.extent = VulkanContext::swapchain->GetExtent();
    renderTargets.accumulationTexture = Details::CreateAccumulationTexture(renderTargets.extent);
    renderTargets.descriptorSet = Details::CreateRenderTargetsDescriptorSet(
            renderTargets.accumulationTexture.view, UseSwapchainRenderTarget());

    cameraData = Details::CreateCameraData(VulkanContext::swapchain->GetImageCount());

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &PathTracingRenderer::HandleKeyInputEvent));

    Engine::AddEventHandler(EventType::eCameraUpdate,
            MakeFunction(this, &PathTracingRenderer::ResetAccumulation));
}

PathTracingRenderer::~PathTracingRenderer()
{
    RemoveScene();

    DescriptorHelpers::DestroyMultiDescriptorSet(cameraData.descriptorSet);
    for (const auto& buffer : cameraData.buffers)
    {
        VulkanContext::bufferManager->DestroyBuffer(buffer);
    }

    DescriptorHelpers::DestroyMultiDescriptorSet(renderTargets.descriptorSet);

    if (AccumulationEnabled())
    {
        VulkanContext::textureManager->DestroyTexture(renderTargets.accumulationTexture);
    }
}

void PathTracingRenderer::RegisterScene(const Scene* scene_)
{
    EASY_FUNCTION()

    RemoveScene();

    ResetAccumulation();

    scene = scene_;

    sceneDescriptorSet = Details::CreateSceneDescriptorSet(*scene);

    rayTracingPipeline = Details::CreateRayTracingPipeline(*scene,
            GetDescriptorSetLayouts(), AccumulationEnabled(),
            isProbeRenderer, sampleCount);
}

void PathTracingRenderer::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    rayTracingPipeline.reset();

    DescriptorHelpers::DestroyDescriptorSet(sceneDescriptorSet);

    scene = nullptr;
}

PathTracingRenderer::PathTracingRenderer(uint32_t sampleCount_, const vk::Extent2D& extent)
    : isProbeRenderer(true)
    , sampleCount(sampleCount_)
{
    renderTargets.extent = extent;
    renderTargets.descriptorSet = Details::CreateRenderTargetsDescriptorSet(
            renderTargets.accumulationTexture.view, UseSwapchainRenderTarget());

    cameraData = Details::CreateCameraData(ImageHelpers::kCubeFaceCount);
}

const CameraComponent& PathTracingRenderer::GetCameraComponent() const
{
    return scene->ctx().get<CameraComponent>();
}

void PathTracingRenderer::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    if (UseSwapchainRenderTarget())
    {
        const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];

        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral,
            PipelineBarrier{
                SyncScope::kWaitForNone,
                SyncScope::kRayTracingShaderWrite
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, swapchainImage,
                ImageHelpers::kFlatColor, layoutTransition);
    }

    if (scene)
    {
        UpdateCameraBuffer(commandBuffer, imageIndex);

        const std::vector<vk::DescriptorSet> descriptorSets{
            renderTargets.descriptorSet.values[imageIndex],
            cameraData.descriptorSet.values[imageIndex],
            sceneDescriptorSet.value
        };

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rayTracingPipeline->Get());

        if (AccumulationEnabled())
        {
            commandBuffer.pushConstants<uint32_t>(rayTracingPipeline->GetLayout(),
                    vk::ShaderStageFlagBits::eRaygenKHR, 0, { accumulationIndex++ });
        }

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR,
                rayTracingPipeline->GetLayout(), 0, descriptorSets, {});

        const ShaderBindingTable& sbt = rayTracingPipeline->GetShaderBindingTable();

        const vk::DeviceAddress bufferAddress = VulkanContext::device->GetAddress(sbt.buffer);

        const vk::StridedDeviceAddressRegionKHR raygenSBT(bufferAddress + sbt.raygenOffset, sbt.stride, sbt.stride);
        const vk::StridedDeviceAddressRegionKHR missSBT(bufferAddress + sbt.missOffset, sbt.stride, sbt.stride);
        const vk::StridedDeviceAddressRegionKHR hitSBT(bufferAddress + sbt.hitOffset, sbt.stride, sbt.stride);

        commandBuffer.traceRaysKHR(raygenSBT, missSBT, hitSBT, vk::StridedDeviceAddressRegionKHR(),
                renderTargets.extent.width, renderTargets.extent.height, 1);
    }

    if (UseSwapchainRenderTarget())
    {
        const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];

        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eColorAttachmentOptimal,
            PipelineBarrier{
                SyncScope::kRayTracingShaderWrite,
                SyncScope::kColorAttachmentWrite
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, swapchainImage,
                ImageHelpers::kFlatColor, layoutTransition);
    }
}

void PathTracingRenderer::Resize(const vk::Extent2D& extent)
{
    Assert(extent.width != 0 && extent.height != 0);

    ResetAccumulation();

    DescriptorHelpers::DestroyMultiDescriptorSet(renderTargets.descriptorSet);

    if (AccumulationEnabled())
    {
        VulkanContext::textureManager->DestroyTexture(renderTargets.accumulationTexture);
    }

    renderTargets.extent = VulkanContext::swapchain->GetExtent();
    renderTargets.accumulationTexture = Details::CreateAccumulationTexture(renderTargets.extent);
    renderTargets.descriptorSet = Details::CreateRenderTargetsDescriptorSet(
            renderTargets.accumulationTexture.view, UseSwapchainRenderTarget());
}

std::vector<vk::DescriptorSetLayout> PathTracingRenderer::GetDescriptorSetLayouts() const
{
    return { renderTargets.descriptorSet.layout, cameraData.descriptorSet.layout, sceneDescriptorSet.layout };
}

void PathTracingRenderer::UpdateCameraBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const gpu::CameraPT cameraShaderData{
        glm::inverse(GetCameraComponent().viewMatrix),
        glm::inverse(GetCameraComponent().projMatrix),
        GetCameraComponent().projection.zNear,
        GetCameraComponent().projection.zFar
    };

    const SyncScope& uniformReadSyncScope = SyncScope::kRayTracingUniformRead;

    BufferHelpers::UpdateBuffer(commandBuffer, cameraData.buffers[imageIndex],
            GetByteView(cameraShaderData), uniformReadSyncScope, uniformReadSyncScope);
}

void PathTracingRenderer::HandleKeyInputEvent(const KeyInput& keyInput)
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

void PathTracingRenderer::ReloadShaders()
{
    VulkanContext::device->WaitIdle();

    ResetAccumulation();

    rayTracingPipeline = Details::CreateRayTracingPipeline(*scene,
            GetDescriptorSetLayouts(), AccumulationEnabled(),
            isProbeRenderer, sampleCount);
}

void PathTracingRenderer::ResetAccumulation()
{
    accumulationIndex = 0;
}
