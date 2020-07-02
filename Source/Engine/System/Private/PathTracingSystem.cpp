#include "Engine/System/PathTracingSystem.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"
#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"
#include "Engine/Scene/SceneRT.hpp"
#include "Engine/Engine.hpp"

namespace Details
{
    std::unique_ptr<RayTracingPipeline> CreateRayTracingPipeline(
            const std::vector<vk::DescriptorSetLayout> &layouts)
    {
        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eRaygenNV,
                    Filepath("~/Shaders/PathTracing/RayGen.rgen")),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eMissNV,
                    Filepath("~/Shaders/PathTracing/Material.rmiss")),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eMissNV,
                    Filepath("~/Shaders/PathTracing/Environment.rmiss")),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eClosestHitNV,
                    Filepath("~/Shaders/PathTracing/Material.rchit"))
        };

        const std::vector<RayTracingPipeline::ShaderGroup> shaderGroups{
            RayTracingPipeline::ShaderGroup{
                vk::RayTracingShaderGroupTypeNV::eGeneral,
                0, VK_SHADER_UNUSED_NV, VK_SHADER_UNUSED_NV
            },
            RayTracingPipeline::ShaderGroup{
                vk::RayTracingShaderGroupTypeNV::eGeneral,
                1, VK_SHADER_UNUSED_NV, VK_SHADER_UNUSED_NV
            },
            RayTracingPipeline::ShaderGroup{
                vk::RayTracingShaderGroupTypeNV::eGeneral,
                2, VK_SHADER_UNUSED_NV, VK_SHADER_UNUSED_NV
            },
            RayTracingPipeline::ShaderGroup{
                vk::RayTracingShaderGroupTypeNV::eTrianglesHitGroup,
                VK_SHADER_UNUSED_NV, 3, VK_SHADER_UNUSED_NV
            },
        };

        const vk::PushConstantRange pushConstant(vk::ShaderStageFlagBits::eRaygenNV, 0, sizeof(uint32_t));

        const RayTracingPipeline::Description description{
            shaderModules, shaderGroups, layouts, { pushConstant }
        };

        return RayTracingPipeline::Create(description);
    }

    void TransitSwapchainImageLayout(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
    {
        const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];

        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral,
            PipelineBarrier{
                SyncScope::kWaitForNothing,
                SyncScope::kRayTracingShaderWrite
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, swapchainImage,
                ImageHelpers::kFlatColor, layoutTransition);
    }
}

PathTracingSystem::PathTracingSystem(SceneRT *scene_)
    : scene(scene_)
{
    SetupRenderTargets();
    SetupAccumulationTarget();
    SetupRayTracingPipeline();
    SetupDescriptorSets();

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &PathTracingSystem::HandleResizeEvent));

    Engine::AddEventHandler(EventType::eCameraUpdate,
            MakeFunction(this, &PathTracingSystem::ResetAccumulation));
}

PathTracingSystem::~PathTracingSystem()
{
    DescriptorHelpers::DestroyMultiDescriptorSet(renderTargets.descriptorSet);
    DescriptorHelpers::DestroyDescriptorSet(accumulationTarget.descriptorSet);
}

void PathTracingSystem::Process(float) {}

void PathTracingSystem::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    scene->UpdateCameraBuffer(commandBuffer);

    Details::TransitSwapchainImageLayout(commandBuffer, imageIndex);

    descriptorSets[0] = renderTargets.descriptorSet.values[imageIndex];

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingNV, rayTracingPipeline->Get());

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingNV,
            rayTracingPipeline->GetLayout(), 0, descriptorSets, {});

    commandBuffer.pushConstants<uint32_t>(rayTracingPipeline->GetLayout(),
            vk::ShaderStageFlagBits::eRaygenNV, 0, { accumulationTarget.accumulationCount++ });

    const ShaderBindingTable &shaderBindingTable = rayTracingPipeline->GetShaderBindingTable();
    const auto &[buffer, raygenOffset, missOffset, hitOffset, stride] = shaderBindingTable;

    const vk::Extent2D &extent = VulkanContext::swapchain->GetExtent();

    commandBuffer.traceRaysNV(buffer, raygenOffset,
            buffer, missOffset, stride,
            buffer, hitOffset, stride,
            nullptr, 0, 0,
            extent.width, extent.height, 1);
}

void PathTracingSystem::SetupRenderTargets()
{
    const std::vector<vk::ImageView> &swapchainImageViews = VulkanContext::swapchain->GetImageViews();

    const DescriptorDescription descriptorDescription{
        1, vk::DescriptorType::eStorageImage,
        vk::ShaderStageFlagBits::eRaygenNV,
        vk::DescriptorBindingFlags()
    };

    std::vector<DescriptorSetData> multiDescriptorData;
    multiDescriptorData.reserve(swapchainImageViews.size());

    for (const auto &swapchainImageView : swapchainImageViews)
    {
        multiDescriptorData.push_back({ DescriptorHelpers::GetData(swapchainImageView) });
    }

    renderTargets.descriptorSet = DescriptorHelpers::CreateMultiDescriptorSet(
            { descriptorDescription }, multiDescriptorData);
}

void PathTracingSystem::SetupAccumulationTarget()
{
    const vk::Extent2D &swapchainExtent = VulkanContext::swapchain->GetExtent();

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

    ImageManager &imageManager = *VulkanContext::imageManager;
    accumulationTarget.image = imageManager.CreateImage(imageDescription, ImageCreateFlags::kNone);
    accumulationTarget.view = imageManager.CreateView(accumulationTarget.image,
            vk::ImageViewType::e2D, ImageHelpers::kFlatColor);

    const DescriptorDescription descriptorDescription{
        1, vk::DescriptorType::eStorageImage,
        vk::ShaderStageFlagBits::eRaygenNV,
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
                    SyncScope::kWaitForNothing,
                    SyncScope::kRayTracingShaderRead | SyncScope::kRayTracingShaderWrite
                }
            };

            ImageHelpers::TransitImageLayout(commandBuffer, accumulationTarget.image,
                    ImageHelpers::kFlatColor, layoutTransition);
        });
}

void PathTracingSystem::SetupRayTracingPipeline()
{
    std::vector<vk::DescriptorSetLayout> layouts{
        renderTargets.descriptorSet.layout,
        accumulationTarget.descriptorSet.layout,
    };

    const std::vector<vk::DescriptorSetLayout> sceneLayouts = scene->GetDescriptorSetLayouts();

    layouts.insert(layouts.begin(), sceneLayouts.begin(), sceneLayouts.end());

    rayTracingPipeline = Details::CreateRayTracingPipeline(layouts);
}

void PathTracingSystem::SetupDescriptorSets()
{
    descriptorSets = std::vector<vk::DescriptorSet>{
        renderTargets.descriptorSet.values.front(),
        accumulationTarget.descriptorSet.value
    };

    const std::vector<vk::DescriptorSet> sceneDescriptorSets = scene->GetDescriptorSets();

    descriptorSets.insert(descriptorSets.begin(),
            sceneDescriptorSets.begin(), sceneDescriptorSets.end());
}

void PathTracingSystem::HandleResizeEvent(const vk::Extent2D &extent)
{
    if (extent.width != 0 && extent.height != 0)
    {
        ResetAccumulation();

        DescriptorHelpers::DestroyMultiDescriptorSet(renderTargets.descriptorSet);
        DescriptorHelpers::DestroyDescriptorSet(accumulationTarget.descriptorSet);

        VulkanContext::imageManager->DestroyImage(accumulationTarget.image);

        SetupRenderTargets();
        SetupAccumulationTarget();
        SetupDescriptorSets();
    }
}

void PathTracingSystem::ResetAccumulation()
{
    accumulationTarget.accumulationCount = 0;
}
