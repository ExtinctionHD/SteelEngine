#include "Engine/Render/PathTracingRenderer.hpp"

#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"
#include "Engine/Render/Vulkan/Pipelines/RayTracingPipeline.hpp"
#include "Engine/Scene/StorageComponents.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Scene/Components.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Config.hpp"
#include "Engine/Engine.hpp"

#include "Shaders/Common/Common.h"

namespace Details
{
    static constexpr uint32_t kSampleCount = 1;

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

    static CameraData CreateCameraData(uint32_t bufferCount)
    {
        constexpr vk::DeviceSize bufferSize = sizeof(gpu::CameraPT);

        constexpr vk::ShaderStageFlags shaderStages = vk::ShaderStageFlagBits::eRaygenKHR;

        return RenderHelpers::CreateCameraData(bufferCount, bufferSize);
    }

    static std::unique_ptr<RayTracingPipeline> CreateRayTracingPipeline(const Scene& scene)
    {
        const auto& materialComponent = scene.ctx().get<MaterialStorageComponent>();

        const uint32_t materialCount = static_cast<uint32_t>(materialComponent.materials.size());

        const uint32_t lightCount = static_cast<uint32_t>(scene.view<LightComponent>().size());

        const ShaderDefines rayGenDefines{
            std::make_pair("ACCUMULATION", 1),
            std::make_pair("RENDER_TO_HDR", 0),
            std::make_pair("RENDER_TO_CUBE", 0),
            std::make_pair("LIGHT_COUNT", lightCount),
            std::make_pair("MATERIAL_COUNT", materialCount),
            std::make_pair("SAMPLE_COUNT", kSampleCount),
        };

        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    Filepath("~/Shaders/PathTracing/RayGen.rgen"),
                    vk::ShaderStageFlagBits::eRaygenKHR,
                    rayGenDefines),
            VulkanContext::shaderManager->CreateShaderModule(
                    Filepath("~/Shaders/PathTracing/Miss.rmiss"),
                    vk::ShaderStageFlagBits::eMissKHR),
            VulkanContext::shaderManager->CreateShaderModule(
                    Filepath("~/Shaders/PathTracing/ClosestHit.rchit"),
                    vk::ShaderStageFlagBits::eClosestHitKHR),
            VulkanContext::shaderManager->CreateShaderModule(
                    Filepath("~/Shaders/PathTracing/AnyHit.rahit"),
                    vk::ShaderStageFlagBits::eAnyHitKHR,
                    { std::make_pair("MATERIAL_COUNT", materialCount) })
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

        const RayTracingPipeline::Description description{ shaderModules, shaderGroupsMap };

        std::unique_ptr<RayTracingPipeline> pipeline = RayTracingPipeline::Create(description);

        for (const auto& shaderModule : shaderModules)
        {
            VulkanContext::shaderManager->DestroyShaderModule(shaderModule);
        }

        return pipeline;
    }
}

PathTracingRenderer::PathTracingRenderer()
{
    EASY_FUNCTION()

    cameraData = Details::CreateCameraData(VulkanContext::swapchain->GetImageCount());

    accumulationTexture = Details::CreateAccumulationTexture(VulkanContext::swapchain->GetExtent());

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &PathTracingRenderer::HandleKeyInputEvent));

    Engine::AddEventHandler(EventType::eCameraUpdate,
            MakeFunction(this, &PathTracingRenderer::ResetAccumulation));
}

PathTracingRenderer::~PathTracingRenderer()
{
    RemoveScene();

    for (const auto& buffer : cameraData.buffers)
    {
        VulkanContext::bufferManager->DestroyBuffer(buffer);
    }

    VulkanContext::textureManager->DestroyTexture(accumulationTexture);
}

void PathTracingRenderer::RegisterScene(const Scene* scene_)
{
    EASY_FUNCTION()

    RemoveScene();

    ResetAccumulation();

    scene = scene_;

    rayTracingPipeline = Details::CreateRayTracingPipeline(*scene);

    CreateDescriptorProvider();
}

void PathTracingRenderer::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    descriptorProvider.FreeDescriptors();

    rayTracingPipeline.reset();

    scene = nullptr;
}

void PathTracingRenderer::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
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

        rayTracingPipeline->Bind(commandBuffer);

        rayTracingPipeline->BindDescriptorSets(commandBuffer, 0, descriptorProvider.GetDescriptorSlice(imageIndex));

        rayTracingPipeline->PushConstant(commandBuffer, "accumIndex", accumulationIndex++);

        const ShaderBindingTable& sbt = rayTracingPipeline->GetShaderBindingTable();

        const vk::DeviceAddress bufferAddress = VulkanContext::device->GetAddress(sbt.buffer);

        const vk::StridedDeviceAddressRegionKHR raygenSBT(bufferAddress + sbt.raygenOffset, sbt.stride, sbt.stride);
        const vk::StridedDeviceAddressRegionKHR missSBT(bufferAddress + sbt.missOffset, sbt.stride, sbt.stride);
        const vk::StridedDeviceAddressRegionKHR hitSBT(bufferAddress + sbt.hitOffset, sbt.stride, sbt.stride);

        const vk::Extent2D extent = VulkanContext::swapchain->GetExtent();

        commandBuffer.traceRaysKHR(raygenSBT, missSBT, hitSBT,
                vk::StridedDeviceAddressRegionKHR(), extent.width, extent.height, 1);
    }

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

    VulkanContext::textureManager->DestroyTexture(accumulationTexture);

    accumulationTexture = Details::CreateAccumulationTexture(VulkanContext::swapchain->GetExtent());
}

void PathTracingRenderer::CreateDescriptorProvider()
{
    descriptorProvider.Allocate(rayTracingPipeline->GetDescriptorSetLayouts());

    const auto& renderComponent = scene->ctx().get<RenderStorageComponent>();
    const auto& textureComponent = scene->ctx().get<TextureStorageComponent>();
    const auto& geometryComponent = scene->ctx().get<GeometryStorageComponent>();
    const auto& environmentComponent = scene->ctx().get<EnvironmentComponent>();

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

    const DescriptorSetData globalDescriptorSetData{
        DescriptorHelpers::GetData(renderComponent.lightBuffer),
        DescriptorHelpers::GetData(renderComponent.materialBuffer),
        DescriptorHelpers::GetData(textureComponent.textures),
        DescriptorHelpers::GetData(RenderContext::defaultSampler, environmentComponent.cubemapTexture.view),
        DescriptorHelpers::GetData(renderComponent.tlas),
        DescriptorHelpers::GetStorageData(indexBuffers),
        DescriptorHelpers::GetStorageData(normalsBuffers),
        DescriptorHelpers::GetStorageData(tangentsBuffers),
        DescriptorHelpers::GetStorageData(texCoordBuffers),
        DescriptorHelpers::GetStorageData(accumulationTexture.view),
    };

    descriptorProvider.UpdateGlobalDescriptorSet(globalDescriptorSetData);

    for (uint32_t i = 0; i < descriptorProvider.GetSliceCount(); ++i)
    {
        const DescriptorSetData frameDescriptorSetData{
            DescriptorHelpers::GetData(cameraData.buffers[i]),
            DescriptorHelpers::GetStorageData(VulkanContext::swapchain->GetImageViews()[i])
        };

        descriptorProvider.UpdateFrameDescriptorSet(i, frameDescriptorSetData);
    }
}

void PathTracingRenderer::UpdateCameraBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const auto& cameraComponent = scene->ctx().get<CameraComponent>();

    const gpu::CameraPT cameraShaderData{
        glm::inverse(cameraComponent.viewMatrix),
        glm::inverse(cameraComponent.projMatrix),
        cameraComponent.projection.zNear,
        cameraComponent.projection.zFar
    };

    BufferHelpers::UpdateBuffer(commandBuffer,
            cameraData.buffers[imageIndex], GetByteView(cameraShaderData),
            SyncScope::kRayTracingUniformRead, SyncScope::kRayTracingUniformRead);
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

    rayTracingPipeline = Details::CreateRayTracingPipeline(*scene);

    CreateDescriptorProvider();
}

void PathTracingRenderer::ResetAccumulation()
{
    accumulationIndex = 0;
}
