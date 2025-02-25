#include "Engine/Render/Stages/AtmosphereStage.hpp"

#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/SceneRenderer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"

namespace Details
{
    static constexpr glm::uvec3 kWorkGroupSize(8, 8, 4);

    static const Filepath kTransmittanceShaderPath("~/Shaders/Compute/Atmosphere/Transmittance.comp");
    static const Filepath kMultiScatteringShaderPath("~/Shaders/Compute/Atmosphere/MultiScattering.comp");
    static const Filepath kArialShaderPath("~/Shaders/Compute/Atmosphere/Arial.comp");
    static const Filepath kSkyShaderPath("~/Shaders/Compute/Atmosphere/Sky.comp");

    static std::unique_ptr<ComputePipeline> CreatePipeline(const Filepath& shaderPath)
    {
        const ShaderModule shaderModule
                = VulkanContext::shaderManager->CreateComputeShaderModule(shaderPath, kWorkGroupSize);

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static void CreateTransmittanceDescriptors(
            DescriptorProvider& descriptorProvider, const SceneRenderContext& context)
    {
        descriptorProvider.PushGlobalData("transmittanceLut", context.atmosphereLUTs.transmittance.image.view);

        for (const auto& frameBuffer : context.uniforms.frames)
        {
            descriptorProvider.PushSliceData("frame", frameBuffer);
        }

        descriptorProvider.FlushData();
    }

    static void CreateMultiScatteringDescriptors(
            DescriptorProvider& descriptorProvider, const SceneRenderContext& context)
    {
        descriptorProvider.PushGlobalData("transmittanceLut", &context.atmosphereLUTs.transmittance);
        descriptorProvider.PushGlobalData("multiScatteringLut", context.atmosphereLUTs.multiScattering.image.view);

        for (const auto& frameBuffer : context.uniforms.frames)
        {
            descriptorProvider.PushSliceData("frame", frameBuffer);
        }

        descriptorProvider.FlushData();
    }

    static void CreateArialDescriptors(
            DescriptorProvider& descriptorProvider, const SceneRenderContext& context)
    {
        descriptorProvider.PushGlobalData("transmittanceLut", &context.atmosphereLUTs.transmittance);
        descriptorProvider.PushGlobalData("multiScatteringLut", &context.atmosphereLUTs.multiScattering);
        descriptorProvider.PushGlobalData("arialLut", context.atmosphereLUTs.arial.image.view);

        for (const auto& frameBuffer : context.uniforms.frames)
        {
            descriptorProvider.PushSliceData("frame", frameBuffer);
        }

        descriptorProvider.FlushData();
    }

    static void CreateSkyDescriptors(
            DescriptorProvider& descriptorProvider, const SceneRenderContext& context)
    {
        descriptorProvider.PushGlobalData("transmittanceLut", &context.atmosphereLUTs.transmittance);
        descriptorProvider.PushGlobalData("multiScatteringLut", &context.atmosphereLUTs.multiScattering);
        descriptorProvider.PushGlobalData("skyLut", context.atmosphereLUTs.sky.image.view);

        for (const auto& frameBuffer : context.uniforms.frames)
        {
            descriptorProvider.PushSliceData("frame", frameBuffer);
        }

        descriptorProvider.FlushData();
    }

    static vk::Extent2D GetTransmittanceLutExtent()
    {
        return VulkanHelpers::GetExtent(RenderOptions::Atmosphere::transmittanceLutExtent);
    }

    static vk::Extent2D GetMultiScatteringLutExtent()
    {
        return VulkanHelpers::GetExtent(RenderOptions::Atmosphere::multiScatteringLutExtent);
    }

    static vk::Extent3D GetArialLutExtent()
    {
        const vk::Extent2D& extent = VulkanHelpers::GetExtent(RenderOptions::Atmosphere::arialLutExtent);
        const auto depth = static_cast<uint32_t>(RenderOptions::Atmosphere::arialLutDepth);

        return VulkanHelpers::GetExtent3D(extent, depth);
    }

    static vk::Extent2D GetSkyLutExtent()
    {
        return VulkanHelpers::GetExtent(RenderOptions::Atmosphere::skyLutExtent);
    }
}

AtmosphereStage::AtmosphereStage(const SceneRenderContext& context_)
    : RenderStage(context_)
{
    pipelines.transmittance = Details::CreatePipeline(Details::kTransmittanceShaderPath);
    pipelines.multiScattering = Details::CreatePipeline(Details::kMultiScatteringShaderPath);
    pipelines.arial = Details::CreatePipeline(Details::kArialShaderPath);
    pipelines.sky = Details::CreatePipeline(Details::kSkyShaderPath);

    descriptorProviders = pipelines.CreateDescriptorProviders();
}

AtmosphereStage::~AtmosphereStage()
{
    AtmosphereStage::RemoveScene();
}

void AtmosphereStage::RegisterScene(const Scene* scene_)
{
    RenderStage::RegisterScene(scene_);

    Details::CreateTransmittanceDescriptors(*descriptorProviders.transmittance, context);
    Details::CreateMultiScatteringDescriptors(*descriptorProviders.multiScattering, context);
    Details::CreateArialDescriptors(*descriptorProviders.arial, context);
    Details::CreateSkyDescriptors(*descriptorProviders.sky, context);
}

void AtmosphereStage::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    descriptorProviders.Clear();

    scene = nullptr;
}

void AtmosphereStage::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const ImageLayoutTransition layoutTransition{
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageLayout::eGeneral,
        PipelineBarrier{
            SyncScope::kComputeShaderRead,
            SyncScope::kComputeShaderWrite,
        },
    };

    for (const auto& [image, sampler] : context.atmosphereLUTs.GetArray())
    {
        ImageHelpers::TransitImageLayout(commandBuffer, image.image,
                ImageHelpers::kFlatColor, layoutTransition);
    }

    RenderTransmittanceLut(commandBuffer, imageIndex);

    RenderMultiTransmittanceLut(commandBuffer, imageIndex);

    RenderArialLut(commandBuffer, imageIndex);

    RenderSkyLut(commandBuffer, imageIndex);
}

void AtmosphereStage::ReloadShaders()
{
    pipelines.transmittance = Details::CreatePipeline(Details::kTransmittanceShaderPath);
    pipelines.multiScattering = Details::CreatePipeline(Details::kMultiScatteringShaderPath);
    pipelines.arial = Details::CreatePipeline(Details::kArialShaderPath);
    pipelines.sky = Details::CreatePipeline(Details::kSkyShaderPath);

    descriptorProviders = pipelines.CreateDescriptorProviders();

    if (scene)
    {
        Details::CreateTransmittanceDescriptors(*descriptorProviders.transmittance, context);
        Details::CreateMultiScatteringDescriptors(*descriptorProviders.multiScattering, context);
        Details::CreateArialDescriptors(*descriptorProviders.arial, context);
        Details::CreateSkyDescriptors(*descriptorProviders.sky, context);
    }
}

void AtmosphereStage::DescriptorProviders::Clear() const
{
    for (const auto& descriptorProvide : GetArray())
    {
        descriptorProvide->Clear();
    }
}

AtmosphereStage::DescriptorProviders AtmosphereStage::Pipelines::CreateDescriptorProviders() const
{
    return DescriptorProviders{
        transmittance->CreateDescriptorProvider(),
        multiScattering->CreateDescriptorProvider(),
        arial->CreateDescriptorProvider(),
        sky->CreateDescriptorProvider(),
    };
}

void AtmosphereStage::RenderTransmittanceLut(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Extent2D extent = Details::GetTransmittanceLutExtent();

    const DescriptorProvider& descriptorProvider = *descriptorProviders.transmittance;

    pipelines.transmittance->Bind(commandBuffer);

    pipelines.transmittance->BindDescriptorSets(commandBuffer, descriptorProvider.GetDescriptorSlice(imageIndex));

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent, Details::kWorkGroupSize);

    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);

    const ImageLayoutTransition layoutTransition{
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        PipelineBarrier{
            SyncScope::kComputeShaderWrite,
            SyncScope::kComputeShaderRead,
        },
    };

    ImageHelpers::TransitImageLayout(commandBuffer,
            context.atmosphereLUTs.transmittance.image.image,
            ImageHelpers::kFlatColor, layoutTransition);
}

void AtmosphereStage::RenderMultiTransmittanceLut(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Extent2D extent = Details::GetMultiScatteringLutExtent();

    const DescriptorProvider& descriptorProvider = *descriptorProviders.multiScattering;

    pipelines.multiScattering->Bind(commandBuffer);

    pipelines.multiScattering->BindDescriptorSets(commandBuffer, descriptorProvider.GetDescriptorSlice(imageIndex));

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent, Details::kWorkGroupSize);

    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);

    const ImageLayoutTransition layoutTransition{
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        PipelineBarrier{
            SyncScope::kComputeShaderWrite,
            SyncScope::kComputeShaderRead,
        },
    };

    ImageHelpers::TransitImageLayout(commandBuffer,
            context.atmosphereLUTs.multiScattering.image.image,
            ImageHelpers::kFlatColor, layoutTransition);
}

void AtmosphereStage::RenderArialLut(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Extent3D extent = Details::GetArialLutExtent();

    const DescriptorProvider& descriptorProvider = *descriptorProviders.arial;

    pipelines.arial->Bind(commandBuffer);

    pipelines.arial->BindDescriptorSets(commandBuffer, descriptorProvider.GetDescriptorSlice(imageIndex));

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent, Details::kWorkGroupSize);

    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);

    const ImageLayoutTransition layoutTransition{
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        PipelineBarrier{
            SyncScope::kComputeShaderWrite,
            SyncScope::kComputeShaderRead,
        },
    };

    ImageHelpers::TransitImageLayout(commandBuffer,
            context.atmosphereLUTs.arial.image.image,
            ImageHelpers::kFlatColor, layoutTransition);
}

void AtmosphereStage::RenderSkyLut(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Extent2D extent = Details::GetSkyLutExtent();

    const DescriptorProvider& descriptorProvider = *descriptorProviders.sky;

    pipelines.sky->Bind(commandBuffer);

    pipelines.sky->BindDescriptorSets(commandBuffer, descriptorProvider.GetDescriptorSlice(imageIndex));

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent, Details::kWorkGroupSize);

    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);

    const ImageLayoutTransition layoutTransition{
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        PipelineBarrier{
            SyncScope::kComputeShaderWrite,
            SyncScope::kComputeShaderRead,
        },
    };

    ImageHelpers::TransitImageLayout(commandBuffer,
            context.atmosphereLUTs.sky.image.image,
            ImageHelpers::kFlatColor, layoutTransition);
}
