#include "Engine/Render/PathTracer.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"
#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"
#include "Engine/Render/Vulkan/ComputeHelpers.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Scene/ScenePT.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Config.hpp"
#include "Engine/Engine.hpp"

#include "Shaders/PathTracing//PathTracing.h"

namespace Details
{
    static constexpr glm::uvec2 kWorkGroupSize(8, 8);

    static constexpr bool IsRayTracingMode()
    {
        return Config::kPathTracingMode == Config::PathTracingMode::eRayTracing;
    }

    static Texture CreateAccumulationTexture(const vk::Extent2D& extent)
    {
        const Texture texture = ImageHelpers::CreateRenderTarget(vk::Format::eR8G8B8A8Unorm,
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

    static std::unique_ptr<RayTracingPipeline> CreateRayTracingPipeline(const ScenePT& scene,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts,
            uint32_t sampleCount, bool accumulation, bool renderToCube)
    {
        const uint32_t pointLightCount = scene.GetInfo().pointLightCount;
        const uint32_t materialCount = scene.GetInfo().materialCount;

        const std::map<std::string, uint32_t> rayGenDefines{
            std::make_pair("ACCUMULATION", accumulation),
            std::make_pair("RENDER_TO_CUBE", renderToCube),
            std::make_pair("POINT_LIGHT_COUNT", pointLightCount)
        };

        std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eRaygenKHR,
                    Filepath("~/Shaders/PathTracing/RayGen.rgen"),
                    rayGenDefines, std::make_tuple(sampleCount, materialCount)),
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

    static std::unique_ptr<ComputePipeline> CreateComputePipeline(const ScenePT& scene,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts,
            uint32_t sampleCount, bool accumulation, bool renderToCube)
    {
        const uint32_t pointLightCount = scene.GetInfo().pointLightCount;
        const uint32_t materialCount = scene.GetInfo().materialCount;

        const std::map<std::string, uint32_t> defines{
            std::make_pair("ACCUMULATION", accumulation),
            std::make_pair("RENDER_TO_CUBE", renderToCube),
            std::make_pair("POINT_LIGHT_COUNT", pointLightCount)
        };

        const std::tuple specializationValues = std::make_tuple(
                kWorkGroupSize.x, kWorkGroupSize.y, 1, sampleCount, materialCount);

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute,
                Filepath("~/Shaders/PathTracing/PathTracing.comp"),
                { defines }, specializationValues);

        const std::vector<vk::PushConstantRange> pushConstantRanges{
            vk::PushConstantRange(vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t))
        };

        const ComputePipeline::Description description{
            shaderModule, descriptorSetLayouts,
            accumulation ? pushConstantRanges : std::vector<vk::PushConstantRange>{}
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(description);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
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
}

PathTracer::PathTracer(ScenePT* scene_, Camera* camera_, Environment* environment_)
    : swapchainRenderTarget(true)
    , accumulationEnabled(true)
    , sampleCount(1)
    , scene(scene_)
    , camera(camera_)
    , environment(environment_)
{
    SetupRenderTargets(VulkanContext::swapchain->GetExtent());

    SetupGeneralData();

    SetupPipeline();

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &PathTracer::HandleResizeEvent));

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &PathTracer::HandleKeyInputEvent));

    Engine::AddEventHandler(EventType::eCameraUpdate,
            MakeFunction(this, &PathTracer::ResetAccumulation));
}

PathTracer::PathTracer(ScenePT* scene_, Camera* camera_, Environment* environment_,
        uint32_t sampleCount_, const vk::Extent2D& extent)
    : swapchainRenderTarget(false)
    , accumulationEnabled(false)
    , sampleCount(sampleCount_)
    , scene(scene_)
    , camera(camera_)
    , environment(environment_)
{
    SetupRenderTargets(extent);

    SetupGeneralData();

    SetupPipeline();
}

PathTracer::~PathTracer()
{
    DescriptorHelpers::DestroyDescriptorSet(generalData.descriptorSet);
    VulkanContext::bufferManager->DestroyBuffer(generalData.cameraBuffer);
    VulkanContext::bufferManager->DestroyBuffer(generalData.directLightBuffer);

    DescriptorHelpers::DestroyMultiDescriptorSet(renderTargets.descriptorSet);

    if (accumulationEnabled)
    {
        VulkanContext::textureManager->DestroyTexture(renderTargets.accumulationTexture);
    }
}

void PathTracer::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    UpdateCameraBuffer(commandBuffer);

    if (swapchainRenderTarget)
    {
        const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];

        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral,
            PipelineBarrier{
                SyncScope::kWaitForNone,
                GetWriteSyncScope()
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, swapchainImage,
                ImageHelpers::kFlatColor, layoutTransition);
    }

    std::vector<vk::DescriptorSet> descriptorSets{
        renderTargets.descriptorSet.values[imageIndex],
        generalData.descriptorSet.value
    };

    for (const auto& [layout, value] : scene->GetDescriptorSets())
    {
        descriptorSets.push_back(value);
    }

    if constexpr (Details::IsRayTracingMode())
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rayTracingPipeline->Get());

        if (accumulationEnabled)
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
    else
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline->Get());

        if (accumulationEnabled)
        {
            commandBuffer.pushConstants<uint32_t>(computePipeline->GetLayout(),
                    vk::ShaderStageFlagBits::eCompute, 0, { accumulationIndex++ });
        }

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                computePipeline->GetLayout(), 0, descriptorSets, {});

        const glm::uvec3 groupCount = ComputeHelpers::CalculateWorkGroupCount(
                renderTargets.extent, Details::kWorkGroupSize);

        commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
    }

    if (swapchainRenderTarget)
    {
        const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];

        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eColorAttachmentOptimal,
            PipelineBarrier{
                GetWriteSyncScope(),
                SyncScope::kColorAttachmentWrite
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, swapchainImage,
                ImageHelpers::kFlatColor, layoutTransition);
    }
}

const SyncScope& PathTracer::GetWriteSyncScope()
{
    if constexpr (Details::IsRayTracingMode())
    {
        return SyncScope::kRayTracingShaderWrite;
    }
    else
    {
        return SyncScope::kComputeShaderWrite;
    }
}

const SyncScope& PathTracer::GetUniformReadSyncScope()
{
    if constexpr (Details::IsRayTracingMode())
    {
        return SyncScope::kRayTracingUniformRead;
    }
    else
    {
        return SyncScope::kComputeUniformRead;
    }
}

void PathTracer::SetupRenderTargets(const vk::Extent2D& extent)
{
    renderTargets.extent = extent;

    DescriptorSetDescription descriptorSetDescription{
        DescriptorDescription{
            1, vk::DescriptorType::eStorageImage,
            Details::GetShaderStages(vk::ShaderStageFlagBits::eRaygenKHR),
            vk::DescriptorBindingFlags()
        }
    };

    if (accumulationEnabled)
    {
        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eStorageImage,
            Details::GetShaderStages(vk::ShaderStageFlagBits::eRaygenKHR),
            vk::DescriptorBindingFlags()
        };

        descriptorSetDescription.push_back(descriptorDescription);

        renderTargets.accumulationTexture = Details::CreateAccumulationTexture(extent);
    }

    if (swapchainRenderTarget)
    {
        const std::vector<vk::ImageView>& swapchainImageViews = VulkanContext::swapchain->GetImageViews();

        std::vector<DescriptorSetData> multiDescriptorSetData;
        multiDescriptorSetData.reserve(swapchainImageViews.size());

        for (const auto& swapchainImageView : swapchainImageViews)
        {
            multiDescriptorSetData.push_back({ DescriptorHelpers::GetData(swapchainImageView) });
        }

        if (accumulationEnabled)
        {
            const DescriptorData descriptorData = DescriptorHelpers::GetData(renderTargets.accumulationTexture.view);

            for (auto& descriptorSetData : multiDescriptorSetData)
            {
                descriptorSetData.push_back(descriptorData);
            }
        }

        renderTargets.descriptorSet = DescriptorHelpers::CreateMultiDescriptorSet(
                descriptorSetDescription, multiDescriptorSetData);
    }
    else
    {
        renderTargets.descriptorSet.layout
                = VulkanContext::descriptorPool->CreateDescriptorSetLayout(descriptorSetDescription);
    }
}

void PathTracer::SetupGeneralData()
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
        DescriptorHelpers::GetData(RenderContext::defaultSampler, environment->GetTexture().view),
    };

    generalData.descriptorSet = DescriptorHelpers::CreateDescriptorSet(
            descriptorSetDescription, descriptorSetData);
}

void PathTracer::SetupPipeline()
{
    std::vector<vk::DescriptorSetLayout> layouts{
        renderTargets.descriptorSet.layout,
        generalData.descriptorSet.layout,
    };

    for (const auto& [layout, value] : scene->GetDescriptorSets())
    {
        layouts.push_back(layout);
    }

    if constexpr (Details::IsRayTracingMode())
    {
        rayTracingPipeline = Details::CreateRayTracingPipeline(*scene, layouts,
                sampleCount, accumulationEnabled, !swapchainRenderTarget);
    }
    else
    {
        computePipeline = Details::CreateComputePipeline(*scene, layouts,
                sampleCount, accumulationEnabled, !swapchainRenderTarget);
    }
}

void PathTracer::UpdateCameraBuffer(vk::CommandBuffer commandBuffer) const
{
    const CameraPT cameraShaderData{
        glm::inverse(camera->GetViewMatrix()),
        glm::inverse(camera->GetProjectionMatrix()),
        camera->GetDescription().zNear,
        camera->GetDescription().zFar
    };

    const SyncScope& uniformReadSyncScope = GetUniformReadSyncScope();

    BufferHelpers::UpdateBuffer(commandBuffer, generalData.cameraBuffer,
            ByteView(cameraShaderData), uniformReadSyncScope, uniformReadSyncScope);
}

void PathTracer::HandleResizeEvent(const vk::Extent2D& extent)
{
    if (extent.width != 0 && extent.height != 0)
    {
        ResetAccumulation();

        DescriptorHelpers::DestroyMultiDescriptorSet(renderTargets.descriptorSet);

        if (accumulationEnabled)
        {
            VulkanContext::textureManager->DestroyTexture(renderTargets.accumulationTexture);
        }

        SetupRenderTargets(VulkanContext::swapchain->GetExtent());
    }
}

void PathTracer::HandleKeyInputEvent(const KeyInput& keyInput)
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

void PathTracer::ReloadShaders()
{
    VulkanContext::device->WaitIdle();

    SetupPipeline();

    ResetAccumulation();
}

void PathTracer::ResetAccumulation()
{
    accumulationIndex = 0;
}