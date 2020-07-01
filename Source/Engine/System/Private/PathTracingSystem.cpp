#include "Engine/System/PathTracingSystem.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"
#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"
#include "Engine/Scene/SceneRT.hpp"
#include "Engine/Engine.hpp"

#include "Shaders/Common/Common.h"
#include "Shaders/PathTracing/PathTracing.h"

namespace SPathTracer
{
    constexpr ShaderData::Lighting kLighting{
        glm::vec4(1.0f, -1.0f, -0.5f, 0.0f),
        glm::vec4(1.0f, 1.0f, 1.0f, 4.0f)
    };

    const Filepath kEnvironmentPath("~/Assets/Textures/SunnyHills.hdr");

    constexpr vk::Extent2D kEnvironmentExtent(2048, 2048);

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

    template <class T>
    DescriptorSet CreateIndexedDescriptor(const DescriptorData &descriptorData)
    {
        const uint32_t descriptorCount = static_cast<uint32_t>(std::get<T>(descriptorData.descriptorInfo).size());

        const DescriptorDescription descriptorDescription{
            descriptorCount, descriptorData.type,
            vk::ShaderStageFlagBits::eClosestHitNV,
            vk::DescriptorBindingFlagBits::eVariableDescriptorCount
        };

        return DescriptorHelpers::CreateDescriptorSet({ descriptorDescription }, { descriptorData });
    }
}

PathTracingSystem::PathTracingSystem(SceneRT *scene_)
    : scene(scene_)
{
    SetupRenderTargets();
    SetupAccumulationTarget();

    
    /*const std::vector<vk::DescriptorSetLayout> rayTracingLayouts{
        renderTargets.multiDescriptor.layout,
        accumulationTarget.descriptor.layout,
        globalUniforms.descriptorSet.layout,
        indexedUniforms.vertexBuffersDescriptor.layout,
        indexedUniforms.indexBuffersDescriptor.layout,
        indexedUniforms.materialBuffersDescriptor.layout,
        indexedUniforms.baseColorTexturesDescriptor.layout,
        indexedUniforms.surfaceTexturesDescriptor.layout,
        indexedUniforms.normalTexturesDescriptor.layout
    };

    rayTracingPipeline = SPathTracer::CreateRayTracingPipeline(rayTracingLayouts);*/
    

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &PathTracingSystem::HandleResizeEvent));

    Engine::AddEventHandler(EventType::eCameraUpdate,
            MakeFunction(this, &PathTracingSystem::ResetAccumulation));
}

PathTracingSystem::~PathTracingSystem()
{
    DescriptorHelpers::DestroyMultiDescriptorSet(renderTargets);
    DescriptorHelpers::DestroyDescriptorSet(accumulationTarget.descriptor);
}

void PathTracingSystem::Process(float) {}

void PathTracingSystem::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    /*const ShaderData::Camera cameraData{
        glm::inverse(scene->GetCamera()->GetViewMatrix()),
        glm::inverse(scene->GetCamera()->GetProjectionMatrix()),
        scene->GetCamera()->GetDescription().zNear,
        scene->GetCamera()->GetDescription().zFar
    };

    BufferHelpers::UpdateBuffer(commandBuffer, globalUniforms.cameraBuffer,
            ByteView(cameraData), SyncScope::kRayTracingShaderRead);*/

    const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];
    const ImageLayoutTransition layoutTransition{
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral,
        PipelineBarrier{
            SyncScope::kWaitForNothing,
            SyncScope::kRayTracingShaderWrite
        }
    };

    ImageHelpers::TransitImageLayout(commandBuffer, swapchainImage, ImageHelpers::kFlatColor, layoutTransition);

    TraceRays(commandBuffer, imageIndex);
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

    renderTargets = DescriptorHelpers::CreateMultiDescriptorSet(
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

    accumulationTarget.descriptor = DescriptorHelpers::CreateDescriptorSet(
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

void PathTracingSystem::TraceRays(vk::CommandBuffer , uint32_t )
{
    /*const std::vector<vk::DescriptorSet> descriptorSets{
        renderTargets.multiDescriptor.values[imageIndex],
        accumulationTarget.descriptor.value,
        globalUniforms.descriptorSet.value,
        indexedUniforms.vertexBuffersDescriptor.value,
        indexedUniforms.indexBuffersDescriptor.value,
        indexedUniforms.materialBuffersDescriptor.value,
        indexedUniforms.baseColorTexturesDescriptor.value,
        indexedUniforms.surfaceTexturesDescriptor.value,
        indexedUniforms.normalTexturesDescriptor.value
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingNV, rayTracingPipeline->Get());

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingNV,
            rayTracingPipeline->GetLayout(), 0, descriptorSets, {});

    commandBuffer.pushConstants(rayTracingPipeline->GetLayout(),
            vk::ShaderStageFlagBits::eRaygenNV, 0, vk::ArrayProxy<const uint32_t>{ accumulationIndex++ });

    const ShaderBindingTable &shaderBindingTable = rayTracingPipeline->GetShaderBindingTable();
    const auto &[buffer, raygenOffset, missOffset, hitOffset, stride] = shaderBindingTable;

    const vk::Extent2D &extent = VulkanContext::swapchain->GetExtent();

    commandBuffer.traceRaysNV(buffer, raygenOffset,
            buffer, missOffset, stride,
            buffer, hitOffset, stride,
            nullptr, 0, 0,
            extent.width, extent.height, 1);*/
}

void PathTracingSystem::HandleResizeEvent(const vk::Extent2D &extent)
{
    if (extent.width != 0 && extent.height != 0)
    {
        ResetAccumulation();

        /*DescriptorHelpers::DestroyMultiDescriptorSet(renderTargets.multiDescriptor);
        DescriptorHelpers::DestroyDescriptorSet(accumulationTarget.descriptor);

        VulkanContext::imageManager->DestroyImage(accumulationTarget.texture.image);*/

        SetupRenderTargets();
        SetupAccumulationTarget();
    }
}

void PathTracingSystem::ResetAccumulation()
{
    accumulationTarget.currentIndex = 0;
}
