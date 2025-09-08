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
    static const Filepath kTransmittanceShaderPath("~/Shaders/Compute/Atmosphere/Transmittance.comp");
    static const Filepath kMultiScatteringShaderPath("~/Shaders/Compute/Atmosphere/MultiScattering.comp");
    static const Filepath kArialShaderPath("~/Shaders/Compute/Atmosphere/Arial.comp");
    static const Filepath kSkyShaderPath("~/Shaders/Compute/Atmosphere/Sky.comp");

    static std::unique_ptr<ComputePipeline> CreatePipeline(const Filepath& shaderPath)
    {
        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(shaderPath);

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
        descriptorProvider.PushGlobalData("lights", context.uniforms.lights);
        descriptorProvider.PushGlobalData("rawSamples", context.atmosphereMisc.rawSamplesBuffer);
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
        descriptorProvider.PushGlobalData("lights", context.uniforms.lights);
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
        descriptorProvider.PushGlobalData("lights", context.uniforms.lights);
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

    Details::CreateTransmittanceDescriptors(*pipelines.transmittance, context);
    Details::CreateMultiScatteringDescriptors(*pipelines.multiScattering, context);
    Details::CreateArialDescriptors(*pipelines.arial, context);
    Details::CreateSkyDescriptors(*pipelines.sky, context);
}

void AtmosphereStage::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
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
    
    Details::CreateTransmittanceDescriptors(*pipelines.transmittance, context);
    Details::CreateMultiScatteringDescriptors(*pipelines.multiScattering, context);
    Details::CreateArialDescriptors(*pipelines.arial, context);
    Details::CreateSkyDescriptors(*pipelines.sky, context);
}

void AtmosphereStage::RenderTransmittanceLut(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Extent2D extent = Details::GetTransmittanceLutExtent();

    pipelines.transmittance->Bind(commandBuffer);

    pipelines.transmittance->BindDescriptorSlice(commandBuffer, imageIndex);

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent);

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

    pipelines.multiScattering->Bind(commandBuffer);

    pipelines.multiScattering->BindDescriptorSlice(commandBuffer, imageIndex);

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent);

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

    pipelines.arial->Bind(commandBuffer);

    pipelines.arial->BindDescriptorSlice(commandBuffer, imageIndex);

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent);

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

    pipelines.sky->Bind(commandBuffer);

    pipelines.sky->BindDescriptorSlice(commandBuffer, imageIndex);

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent);

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
