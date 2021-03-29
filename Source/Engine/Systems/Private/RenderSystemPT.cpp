#include "Engine/Systems/RenderSystemPT.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"
#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"
#include "Engine/Render/Vulkan/ComputeHelpers.hpp"
#include "Engine/Scene/ScenePT.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Config.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Render/Renderer.hpp"
#include "Shaders/PathTracing//PathTracing.h"

namespace Details
{
    static constexpr glm::uvec2 kWorkGroupSize(8, 8);

    static std::unique_ptr<RayTracingPipeline> CreateRayTracingPipeline(const ScenePT& scene,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
    {
        const uint32_t pointLightCount = scene.GetInfo().pointLightCount;
        const uint32_t materialCount = scene.GetInfo().materialCount;

        const std::map<std::string, uint32_t> rayGenDefines{
            std::make_pair("SAMPLE_COUNT", Config::kPathTracingSampleCount),
            std::make_pair("POINT_LIGHT_COUNT", pointLightCount)
        };

        std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eRaygenKHR,
                    Filepath("~/Shaders/PathTracing/RayGen.rgen"),
                    rayGenDefines, std::make_tuple(materialCount)),
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

        if (scene.GetInfo().pointLightCount > 0)
        {
            shaderModules.push_back(VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eMissKHR,
                    Filepath("~/Shaders/PathTracing/Miss.rmiss"),
                    { std::make_pair("PAYLOAD_LOCATION", 1) }));
            shaderModules.push_back(VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eClosestHitKHR,
                    Filepath("~/Shaders/PathTracing/PointLights.rchit"),
                    { std::make_pair("POINT_LIGHT_COUNT", pointLightCount) }));
            shaderModules.push_back(VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eIntersectionKHR,
                    Filepath("~/Shaders/PathTracing/Sphere.rint"), {}));

            shaderGroupsMap[ShaderGroupType::eMiss].push_back(ShaderGroup{
                4, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR
            });
            shaderGroupsMap[ShaderGroupType::eHit].push_back(ShaderGroup{
                VK_SHADER_UNUSED_KHR, 5, VK_SHADER_UNUSED_KHR, 6
            });
        }

        const vk::PushConstantRange pushConstantRange(
                vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(uint32_t));

        const RayTracingPipeline::Description description{
            shaderModules, shaderGroupsMap, descriptorSetLayouts, { pushConstantRange }
        };

        std::unique_ptr<RayTracingPipeline> pipeline = RayTracingPipeline::Create(description);

        for (const auto& shaderModule : shaderModules)
        {
            VulkanContext::shaderManager->DestroyShaderModule(shaderModule);
        }

        return pipeline;
    }

    static std::unique_ptr<ComputePipeline> CreateComputePipeline(const ScenePT& scene,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
    {
        const std::map<std::string, uint32_t> defines{
            std::make_pair("SAMPLE_COUNT", Config::kPathTracingSampleCount),
            std::make_pair("POINT_LIGHT_COUNT", scene.GetInfo().pointLightCount)
        };

        const std::tuple specializationValues = std::make_tuple(
                kWorkGroupSize.x, kWorkGroupSize.y, 1, scene.GetInfo().materialCount);

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute,
                Filepath("~/Shaders/PathTracing/PathTracing.comp"),
                { defines }, specializationValues);

        const vk::PushConstantRange pushConstantRange(
                vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t));

        const ComputePipeline::Description description{
            shaderModule, descriptorSetLayouts, { pushConstantRange }
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(description);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static constexpr bool IsRayTracingMode()
    {
        return Config::kPathTracingMode == Config::PathTracingMode::eRayTracing;
    }

    static constexpr vk::ShaderStageFlags GetShaderStages(vk::ShaderStageFlags shaderStages)
    {
        if constexpr (IsRayTracingMode())
        {
            return shaderStages;
        }
        else
        {
            return vk::ShaderStageFlagBits::eCompute;
        }
    }

    static const SyncScope& GetWriteSyncScope()
    {
        if constexpr (IsRayTracingMode())
        {
            return SyncScope::kRayTracingShaderWrite;
        }
        else
        {
            return SyncScope::kComputeShaderWrite;
        }
    }

    static const SyncScope& GetUniformReadSyncScope()
    {
        if constexpr (IsRayTracingMode())
        {
            return SyncScope::kRayTracingUniformRead;
        }
        else
        {
            return SyncScope::kComputeUniformRead;
        }
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
    SetupPipeline();

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &RenderSystemPT::HandleResizeEvent));

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &RenderSystemPT::HandleKeyInputEvent));

    Engine::AddEventHandler(EventType::eCameraUpdate,
            MakeFunction(this, &RenderSystemPT::ResetAccumulation));
}

RenderSystemPT::~RenderSystemPT()
{
    DescriptorHelpers::DestroyDescriptorSet(generalData.descriptorSet);
    VulkanContext::bufferManager->DestroyBuffer(generalData.cameraBuffer);
    VulkanContext::bufferManager->DestroyBuffer(generalData.directLightBuffer);

    DescriptorHelpers::DestroyDescriptorSet(accumulationTarget.descriptorSet);
    VulkanContext::imageManager->DestroyImage(accumulationTarget.image);

    DescriptorHelpers::DestroyMultiDescriptorSet(renderTargets.descriptorSet);
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
                Details::GetWriteSyncScope()
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, swapchainImage,
                ImageHelpers::kFlatColor, layoutTransition);
    }

    std::vector<vk::DescriptorSet> descriptorSets{
        renderTargets.descriptorSet.values[imageIndex],
        accumulationTarget.descriptorSet.value,
        generalData.descriptorSet.value
    };

    for (const auto& [layout, value] : scene->GetDescriptorSets())
    {
        descriptorSets.push_back(value);
    }

    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    if constexpr (Details::IsRayTracingMode())
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rayTracingPipeline->Get());

        commandBuffer.pushConstants<uint32_t>(rayTracingPipeline->GetLayout(),
                vk::ShaderStageFlagBits::eRaygenKHR, 0, { accumulationTarget.accumulationCount });

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR,
                rayTracingPipeline->GetLayout(), 0, descriptorSets, {});

        const ShaderBindingTable& sbt = rayTracingPipeline->GetShaderBindingTable();

        const vk::DeviceAddress bufferAddress = VulkanContext::device->GetAddress(sbt.buffer);

        const vk::StridedDeviceAddressRegionKHR raygenSBT(bufferAddress + sbt.raygenOffset, sbt.stride, sbt.stride);
        const vk::StridedDeviceAddressRegionKHR missSBT(bufferAddress + sbt.missOffset, sbt.stride, sbt.stride);
        const vk::StridedDeviceAddressRegionKHR hitSBT(bufferAddress + sbt.hitOffset, sbt.stride, sbt.stride);

        commandBuffer.traceRaysKHR(raygenSBT, missSBT, hitSBT,
                vk::StridedDeviceAddressRegionKHR(), extent.width, extent.height, 1);
    }
    else
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline->Get());

        commandBuffer.pushConstants<uint32_t>(computePipeline->GetLayout(),
                vk::ShaderStageFlagBits::eCompute, 0, { accumulationTarget.accumulationCount });

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                computePipeline->GetLayout(), 0, descriptorSets, {});

        const glm::uvec3 groupCount = ComputeHelpers::CalculateWorkGroupCount(extent, Details::kWorkGroupSize);

        commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
    }

    if constexpr (Config::kPathTracingAccumulation)
    {
        ++accumulationTarget.accumulationCount;
    }

    {
        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eColorAttachmentOptimal,
            PipelineBarrier{
                Details::GetWriteSyncScope(),
                SyncScope::kColorAttachmentWrite
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, swapchainImage,
                ImageHelpers::kFlatColor, layoutTransition);
    }
}

void RenderSystemPT::SetupRenderTargets()
{
    constexpr vk::ShaderStageFlags shaderStages = Details::GetShaderStages(vk::ShaderStageFlagBits::eRaygenKHR);

    renderTargets.descriptorSet = DescriptorHelpers::CreateSwapchainDescriptorSet(shaderStages);
}

void RenderSystemPT::SetupAccumulationTarget()
{
    const vk::Extent2D& swapchainExtent = VulkanContext::swapchain->GetExtent();

    const auto& [image, view] = ImageHelpers::CreateRenderTarget(vk::Format::eR8G8B8A8Unorm,
            swapchainExtent, vk::SampleCountFlagBits::e1, vk::ImageUsageFlagBits::eStorage);

    accumulationTarget.image = image;
    accumulationTarget.view = view;

    const DescriptorDescription descriptorDescription{
        1, vk::DescriptorType::eStorageImage,
        Details::GetShaderStages(vk::ShaderStageFlagBits::eRaygenKHR),
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

    generalData.cameraBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eUniformBuffer, ByteView(CameraPT{}));

    generalData.directLightBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eUniformBuffer, ByteView(directLight));

    const DescriptorSetDescription descriptorSetDescription{
        DescriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            Details::GetShaderStages(vk::ShaderStageFlagBits::eRaygenKHR),
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            Details::GetShaderStages(vk::ShaderStageFlagBits::eRaygenKHR),
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            Details::GetShaderStages(vk::ShaderStageFlagBits::eRaygenKHR),
            vk::DescriptorBindingFlags()
        }
    };

    const DescriptorSetData descriptorSetData{
        DescriptorHelpers::GetData(generalData.cameraBuffer),
        DescriptorHelpers::GetData(generalData.directLightBuffer),
        DescriptorHelpers::GetData(Renderer::defaultSampler, environment->GetTexture().view),
    };

    generalData.descriptorSet = DescriptorHelpers::CreateDescriptorSet(
            descriptorSetDescription, descriptorSetData);
}

void RenderSystemPT::SetupPipeline()
{
    std::vector<vk::DescriptorSetLayout> layouts{
        renderTargets.descriptorSet.layout,
        accumulationTarget.descriptorSet.layout,
        generalData.descriptorSet.layout,
    };

    for (const auto& [layout, value] : scene->GetDescriptorSets())
    {
        layouts.push_back(layout);
    }

    if constexpr (Details::IsRayTracingMode())
    {
        rayTracingPipeline = Details::CreateRayTracingPipeline(*scene, layouts);
    }
    else
    {
        computePipeline = Details::CreateComputePipeline(*scene, layouts);
    }
}

void RenderSystemPT::UpdateCameraBuffer(vk::CommandBuffer commandBuffer) const
{
    const CameraPT cameraShaderData{
        glm::inverse(camera->GetViewMatrix()),
        glm::inverse(camera->GetProjectionMatrix()),
        camera->GetDescription().zNear,
        camera->GetDescription().zFar
    };

    const SyncScope& uniformReadSyncScope = Details::GetUniformReadSyncScope();

    BufferHelpers::UpdateBuffer(commandBuffer, generalData.cameraBuffer,
            ByteView(cameraShaderData), uniformReadSyncScope, uniformReadSyncScope);
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

    SetupPipeline();

    ResetAccumulation();
}

void RenderSystemPT::ResetAccumulation()
{
    accumulationTarget.accumulationCount = 0;
}
