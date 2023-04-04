#include "Engine/Render/Stages/LightingStage.hpp"

#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Stages/GBufferStage.hpp"
#include "Engine/Render/Vulkan/PipelineHelpers.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Scene/GlobalIllumination.hpp"
#include "Engine/Scene/StorageComponents.hpp"
#include "Engine/Scene/Components.hpp"
#include "Engine/Scene/Scene.hpp"

namespace Details
{
    static constexpr glm::uvec2 kWorkGroupSize(8, 8);

    static DescriptorSet CreateGBufferDescriptorSet(const std::vector<vk::ImageView>& imageViews)
    {
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

        DescriptorSetDescription descriptorSetDescription(imageViews.size());
        DescriptorSetData descriptorSetData(imageViews.size());

        for (size_t i = 0; i < imageViews.size(); ++i)
        {
            if (ImageHelpers::IsDepthFormat(GBufferStage::kFormats[i]))
            {
                descriptorSetDescription[i] = sampledImageDescriptorDescription;
                descriptorSetData[i] = DescriptorHelpers::GetData(RenderContext::texelSampler, imageViews[i]);
            }
            else
            {
                descriptorSetDescription[i] = storageImageDescriptorDescription;
                descriptorSetData[i] = DescriptorHelpers::GetStorageData(imageViews[i]);
            }
        }

        return DescriptorHelpers::CreateDescriptorSet(descriptorSetDescription, descriptorSetData);
    }

    static MultiDescriptorSet CreateSwapchainDescriptorSet()
    {
        const std::vector<vk::ImageView>& swapchainImageViews = VulkanContext::swapchain->GetImageViews();

        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eStorageImage,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        std::vector<DescriptorSetData> multiDescriptorSetData;
        multiDescriptorSetData.reserve(swapchainImageViews.size());

        for (const auto& swapchainImageView : swapchainImageViews)
        {
            multiDescriptorSetData.push_back({ DescriptorHelpers::GetStorageData(swapchainImageView) });
        }

        return DescriptorHelpers::CreateMultiDescriptorSet({ descriptorDescription }, multiDescriptorSetData);
    }

    static CameraData CreateCameraData()
    {
        const uint32_t bufferCount = VulkanContext::swapchain->GetImageCount();

        constexpr vk::DeviceSize bufferSize = sizeof(glm::mat4);

        constexpr vk::ShaderStageFlags shaderStages = vk::ShaderStageFlagBits::eCompute;

        return RenderHelpers::CreateCameraData(bufferCount, bufferSize, shaderStages);
    }

    static std::unique_ptr<ComputePipeline> CreatePipeline(const Scene& scene,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
    {
        const auto& materialComponent = scene.ctx().get<MaterialStorageComponent>();

        const bool lightVolumeEnabled = scene.ctx().contains<LightVolumeComponent>();

        const std::tuple specializationValues = std::make_tuple(kWorkGroupSize.x, kWorkGroupSize.y);

        const ShaderDefines defines{
            std::make_pair("LIGHT_COUNT", static_cast<uint32_t>(scene.view<LightComponent>().size())),
            std::make_pair("MATERIAL_COUNT", static_cast<uint32_t>(materialComponent.materials.size())),
            std::make_pair("RAY_TRACING_MATERIAL_COUNT", static_cast<uint32_t>(materialComponent.materials.size())),
            std::make_pair("RAY_TRACING_ENABLED", static_cast<uint32_t>(Config::kRayTracingEnabled)),
            std::make_pair("LIGHT_VOLUME_ENABLED", static_cast<uint32_t>(lightVolumeEnabled)),
        };

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute, Filepath("~/Shaders/Hybrid/Lighting.comp"),
                defines, specializationValues);

        const vk::PushConstantRange pushConstantRange(
                vk::ShaderStageFlagBits::eCompute, 0, sizeof(glm::vec3));

        const ComputePipeline::Description description{
            shaderModule, descriptorSetLayouts, { pushConstantRange }
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(description);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }
}

LightingStage::LightingStage(const std::vector<vk::ImageView>& gBufferImageViews)
{
    gBufferDescriptorSet = Details::CreateGBufferDescriptorSet(gBufferImageViews);

    swapchainDescriptorSet = Details::CreateSwapchainDescriptorSet();

    cameraData = Details::CreateCameraData();
}

LightingStage::~LightingStage()
{
    RemoveScene();

    DescriptorHelpers::DestroyMultiDescriptorSet(cameraData.descriptorSet);
    for (const auto& buffer : cameraData.buffers)
    {
        VulkanContext::bufferManager->DestroyBuffer(buffer);
    }

    DescriptorHelpers::DestroyDescriptorSet(gBufferDescriptorSet);
    DescriptorHelpers::DestroyMultiDescriptorSet(swapchainDescriptorSet);
}

void LightingStage::RegisterScene(const Scene* scene_)
{
    RemoveScene();

    scene = scene_;

    lightingDescriptorSet = RenderHelpers::CreateLightingDescriptorSet(
            *scene, vk::ShaderStageFlagBits::eCompute);

    if constexpr (Config::kRayTracingEnabled)
    {
        rayTracingDescriptorSet = RenderHelpers::CreateRayTracingDescriptorSet(
                *scene, vk::ShaderStageFlagBits::eCompute, true);
    }

    pipeline = Details::CreatePipeline(*scene, GetDescriptorSetLayouts());
}

void LightingStage::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    pipeline.reset();

    DescriptorHelpers::DestroyDescriptorSet(lightingDescriptorSet);
    DescriptorHelpers::DestroyDescriptorSet(rayTracingDescriptorSet);

    scene = nullptr;
}

void LightingStage::Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const auto& cameraComponent = scene->ctx().get<CameraComponent>();

    const glm::mat4& view = cameraComponent.viewMatrix;
    const glm::mat4& proj = cameraComponent.projMatrix;

    const glm::mat4 inverseProjView = glm::inverse(view) * glm::inverse(proj);

    BufferHelpers::UpdateBuffer(commandBuffer, cameraData.buffers[imageIndex],
            GetByteView(inverseProjView), SyncScope::kWaitForNone, SyncScope::kComputeShaderRead);

    const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];
    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();
    const glm::vec3& cameraPosition = cameraComponent.location.position;

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

    std::vector<vk::DescriptorSet> descriptorSets{
        swapchainDescriptorSet.values[imageIndex],
        gBufferDescriptorSet.value,
        cameraData.descriptorSet.values[imageIndex],
        lightingDescriptorSet.value,
    };

    if constexpr (Config::kRayTracingEnabled)
    {
        descriptorSets.push_back(rayTracingDescriptorSet.value);
    }

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline->Get());

    commandBuffer.pushConstants<glm::vec3>(pipeline->GetLayout(),
            vk::ShaderStageFlagBits::eCompute, 0, { cameraPosition });

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
            pipeline->GetLayout(), 0, descriptorSets, {});

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent, Details::kWorkGroupSize);

    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
}

void LightingStage::Resize(const std::vector<vk::ImageView>& gBufferImageViews)
{
    DescriptorHelpers::DestroyDescriptorSet(gBufferDescriptorSet);
    DescriptorHelpers::DestroyMultiDescriptorSet(swapchainDescriptorSet);

    gBufferDescriptorSet = Details::CreateGBufferDescriptorSet(gBufferImageViews);
    swapchainDescriptorSet = Details::CreateSwapchainDescriptorSet();

    pipeline = Details::CreatePipeline(*scene, GetDescriptorSetLayouts());
}

void LightingStage::ReloadShaders()
{
    pipeline = Details::CreatePipeline(*scene, GetDescriptorSetLayouts());
}

std::vector<vk::DescriptorSetLayout> LightingStage::GetDescriptorSetLayouts() const
{
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts{
        swapchainDescriptorSet.layout,
        gBufferDescriptorSet.layout,
        cameraData.descriptorSet.layout,
        lightingDescriptorSet.layout,
    };

    if constexpr (Config::kRayTracingEnabled)
    {
        descriptorSetLayouts.push_back(rayTracingDescriptorSet.layout);
    }

    return descriptorSetLayouts;
}
