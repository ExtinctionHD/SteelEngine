#include "Engine/System/RenderSystemPT.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"
#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"
#include "Engine/Scene/ScenePT.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Config.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Render/Renderer.hpp"
#include "Shaders/PathTracing//PathTracing.h"

namespace Details
{
    static std::unique_ptr<RayTracingPipeline> CreateRayTracingPipeline(const ScenePT& scene,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
    {
        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eRaygenKHR,
                    Filepath("~/Shaders/PathTracing/RayGen.rgen"), {},
                    std::make_tuple(scene.GetInfo().materialCount)),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eMissKHR,
                    Filepath("~/Shaders/PathTracing/Miss.rmiss"), {}),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eClosestHitKHR,
                    Filepath("~/Shaders/PathTracing/ClosestHit.rchit"), {}),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eAnyHitKHR,
                    Filepath("~/Shaders/PathTracing/AnyHit.rahit"), {},
                    std::make_tuple(scene.GetInfo().materialCount))
        };

        const std::vector<RayTracingPipeline::ShaderGroup> shaderGroups{
            RayTracingPipeline::ShaderGroup{
                vk::RayTracingShaderGroupTypeKHR::eGeneral,
                0, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR
            },
            RayTracingPipeline::ShaderGroup{
                vk::RayTracingShaderGroupTypeKHR::eGeneral,
                1, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR
            },
            RayTracingPipeline::ShaderGroup{
                vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
                VK_SHADER_UNUSED_KHR, 2, 3
            }
        };

        const vk::PushConstantRange pushConstantRange(
                vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(uint32_t));

        const RayTracingPipeline::Description description{
            shaderModules, shaderGroups, descriptorSetLayouts, { pushConstantRange }
        };

        std::unique_ptr<RayTracingPipeline> pipeline = RayTracingPipeline::Create(description);

        for (const auto& shaderModule : shaderModules)
        {
            VulkanContext::shaderManager->DestroyShaderModule(shaderModule);
        }

        return pipeline;
    }
}

RenderSystemPT::RenderSystemPT(ScenePT* scene_, Camera* camera_, Environment* environment_)
    : scene(scene_)
    , camera(camera_)
    , environment(environment_)
{
    SetupRenderTargets();
    SetupAccumulationTarget();

    SetupGeneralData();

    SetupRayTracingPipeline();

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &RenderSystemPT::HandleResizeEvent));

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &RenderSystemPT::HandleKeyInputEvent));

    Engine::AddEventHandler(EventType::eCameraUpdate,
            MakeFunction(this, &RenderSystemPT::ResetAccumulation));
}

RenderSystemPT::~RenderSystemPT()
{
    DescriptorHelpers::DestroyMultiDescriptorSet(renderTargets.descriptorSet);
    DescriptorHelpers::DestroyDescriptorSet(accumulationTarget.descriptorSet);

    DescriptorHelpers::DestroyDescriptorSet(generalData.descriptorSet);

    VulkanContext::bufferManager->DestroyBuffer(generalData.cameraBuffer);
    VulkanContext::bufferManager->DestroyBuffer(generalData.lightingBuffer);

    VulkanContext::imageManager->DestroyImage(accumulationTarget.image);
}

void RenderSystemPT::Process(float) {}

void RenderSystemPT::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    UpdateCameraBuffer(commandBuffer);

    const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];

    {

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

    const std::vector<vk::DescriptorSet> descriptorSets{
        renderTargets.descriptorSet.values[imageIndex],
        accumulationTarget.descriptorSet.value,
        generalData.descriptorSet.value,
        scene->GetDescriptorSet().value
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rayTracingPipeline->Get());

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR,
            rayTracingPipeline->GetLayout(), 0, descriptorSets, {});

    commandBuffer.pushConstants<uint32_t>(rayTracingPipeline->GetLayout(),
            vk::ShaderStageFlagBits::eRaygenKHR, 0, { accumulationTarget.accumulationCount++ });

    const ShaderBindingTable& sbt = rayTracingPipeline->GetShaderBindingTable();

    const vk::DeviceAddress bufferAddress = VulkanContext::device->GetAddress(sbt.buffer);

    const vk::StridedDeviceAddressRegionKHR raygenSBT(bufferAddress + sbt.raygenOffset, sbt.stride, sbt.stride);
    const vk::StridedDeviceAddressRegionKHR missSBT(bufferAddress + sbt.missOffset, sbt.stride, sbt.stride);
    const vk::StridedDeviceAddressRegionKHR hitSBT(bufferAddress + sbt.hitOffset, sbt.stride, sbt.stride);

    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    commandBuffer.traceRaysKHR(raygenSBT, missSBT, hitSBT,
            vk::StridedDeviceAddressRegionKHR(), extent.width, extent.height, 1);

    {
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

void RenderSystemPT::SetupRenderTargets()
{
    const std::vector<vk::ImageView>& swapchainImageViews = VulkanContext::swapchain->GetImageViews();

    const DescriptorDescription descriptorDescription{
        1, vk::DescriptorType::eStorageImage,
        vk::ShaderStageFlagBits::eRaygenKHR,
        vk::DescriptorBindingFlags()
    };

    std::vector<DescriptorSetData> multiDescriptorData;
    multiDescriptorData.reserve(swapchainImageViews.size());

    for (const auto& swapchainImageView : swapchainImageViews)
    {
        multiDescriptorData.push_back({ DescriptorHelpers::GetData(swapchainImageView) });
    }

    renderTargets.descriptorSet = DescriptorHelpers::CreateMultiDescriptorSet(
            { descriptorDescription }, multiDescriptorData);
}

void RenderSystemPT::SetupAccumulationTarget()
{
    const vk::Extent2D& swapchainExtent = VulkanContext::swapchain->GetExtent();

    const ImageDescription imageDescription{
        ImageType::e2D,
        vk::Format::eR8G8B8A8Unorm,
        VulkanHelpers::GetExtent3D(swapchainExtent),
        1, 1, vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eStorage,
        vk::ImageLayout::eUndefined,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    ImageManager& imageManager = *VulkanContext::imageManager;
    accumulationTarget.image = imageManager.CreateImage(imageDescription, ImageCreateFlags::kNone);
    accumulationTarget.view = imageManager.CreateView(accumulationTarget.image,
            vk::ImageViewType::e2D, ImageHelpers::kFlatColor);

    const DescriptorDescription descriptorDescription{
        1, vk::DescriptorType::eStorageImage,
        vk::ShaderStageFlagBits::eRaygenKHR,
        vk::DescriptorBindingFlags()
    };

    const DescriptorData descriptorData = DescriptorHelpers::GetData(accumulationTarget.view);

    accumulationTarget.descriptorSet = DescriptorHelpers::CreateDescriptorSet(
            { descriptorDescription }, { descriptorData });

    VulkanContext::device->ExecuteOneTimeCommands([this](vk::CommandBuffer commandBuffer)
        {
            const ImageLayoutTransition layoutTransition{
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eGeneral,
                PipelineBarrier{
                    SyncScope::kWaitForNone,
                    SyncScope::kBlockNone
                }
            };

            ImageHelpers::TransitImageLayout(commandBuffer, accumulationTarget.image,
                    ImageHelpers::kFlatColor, layoutTransition);
        });
}

void RenderSystemPT::SetupGeneralData()
{
    const DirectLight& directLight = environment->GetDirectLight();

    const BufferDescription bufferDescription{
        sizeof(CameraPT),
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    generalData.cameraBuffer = VulkanContext::bufferManager->CreateBuffer(
            bufferDescription, BufferCreateFlagBits::eStagingBuffer);

    generalData.lightingBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eUniformBuffer, ByteView(directLight));

    const DescriptorSetDescription descriptorSetDescription{
        DescriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eRaygenKHR,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eRaygenKHR,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eMissKHR,
            vk::DescriptorBindingFlags()
        }
    };

    const DescriptorSetData descriptorSetData{
        DescriptorHelpers::GetData(generalData.cameraBuffer),
        DescriptorHelpers::GetData(generalData.lightingBuffer),
        DescriptorHelpers::GetData(Renderer::defaultSampler, environment->GetTexture().view)
    };

    generalData.descriptorSet = DescriptorHelpers::CreateDescriptorSet(
            descriptorSetDescription, descriptorSetData);
}

void RenderSystemPT::SetupRayTracingPipeline()
{
    const std::vector<vk::DescriptorSetLayout> layouts{
        renderTargets.descriptorSet.layout,
        accumulationTarget.descriptorSet.layout,
        generalData.descriptorSet.layout,
        scene->GetDescriptorSet().layout
    };

    rayTracingPipeline = Details::CreateRayTracingPipeline(*scene, layouts);
}

void RenderSystemPT::UpdateCameraBuffer(vk::CommandBuffer commandBuffer) const
{
    const CameraPT cameraShaderData{
        glm::inverse(camera->GetViewMatrix()),
        glm::inverse(camera->GetProjectionMatrix()),
        camera->GetDescription().zNear,
        camera->GetDescription().zFar
    };

    BufferHelpers::UpdateBuffer(commandBuffer, generalData.cameraBuffer,
            ByteView(cameraShaderData), SyncScope::kRayTracingShaderRead);
}

void RenderSystemPT::HandleResizeEvent(const vk::Extent2D& extent)
{
    if (extent.width != 0 && extent.height != 0)
    {
        ResetAccumulation();

        DescriptorHelpers::DestroyMultiDescriptorSet(renderTargets.descriptorSet);
        DescriptorHelpers::DestroyDescriptorSet(accumulationTarget.descriptorSet);

        VulkanContext::imageManager->DestroyImage(accumulationTarget.image);

        SetupRenderTargets();
        SetupAccumulationTarget();
    }
}

void RenderSystemPT::HandleKeyInputEvent(const KeyInput& keyInput)
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

void RenderSystemPT::ReloadShaders()
{
    VulkanContext::device->WaitIdle();

    SetupRayTracingPipeline();

    ResetAccumulation();
}

void RenderSystemPT::ResetAccumulation()
{
    accumulationTarget.accumulationCount = 0;
}
