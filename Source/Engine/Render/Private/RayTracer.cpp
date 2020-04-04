#include "Engine/Render/RayTracer.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCache.hpp"
#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Camera.hpp"

namespace SRayTracer
{
    std::unique_ptr<RayTracingPipeline> CreateRayTracingPipeline(
            const std::vector<vk::DescriptorSetLayout> &descriptorSetLayouts)
    {
        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderCache->CreateShaderModule(
                    vk::ShaderStageFlagBits::eRaygenNV, Filepath("~/Shaders/RayTrace.rgen"), {}),
            VulkanContext::shaderCache->CreateShaderModule(
                    vk::ShaderStageFlagBits::eMissNV, Filepath("~/Shaders/RayTrace.rmiss"), {}),
            VulkanContext::shaderCache->CreateShaderModule(
                    vk::ShaderStageFlagBits::eClosestHitNV, Filepath("~/Shaders/RayTrace.rchit"), {})
        };

        const std::vector<RayTracingShaderGroup> shaderGroups{
            { vk::RayTracingShaderGroupTypeNV::eGeneral, 0, VK_SHADER_UNUSED_NV, VK_SHADER_UNUSED_NV },
            { vk::RayTracingShaderGroupTypeNV::eGeneral, 1, VK_SHADER_UNUSED_NV, VK_SHADER_UNUSED_NV },
            { vk::RayTracingShaderGroupTypeNV::eTrianglesHitGroup, VK_SHADER_UNUSED_NV, 2, VK_SHADER_UNUSED_NV },
        };

        const RayTracingPipelineDescription description{
            shaderModules, shaderGroups, descriptorSetLayouts, {}
        };

        return RayTracingPipeline::Create(description);
    }
}

RayTracer::RayTracer(Scene &scene_, Camera &camera_)
    : scene(scene_)
    , camera(camera_)
{
    SetupRenderObjects();
    SetupRenderTargets();
    SetupGlobalUniforms();

    rayTracingPipeline = SRayTracer::CreateRayTracingPipeline({ renderTargetLayout, globalLayout });
}

void RayTracer::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    BufferHelpers::UpdateUniformBuffer(commandBuffer, globalUniforms.cameraBuffer,
            GetByteView(camera.GetData()), SyncScope::kRayTracingShaderRead);

    const ImageLayoutTransition initialLayoutTransition{
        vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
        PipelineBarrier{
            SyncScope::kWaitForNothing,
            SyncScope::kRayTracingShaderWrite
        }
    };
    ImageHelpers::TransitImageLayout(commandBuffer,
            VulkanContext::swapchain->GetImages()[imageIndex],
            ImageHelpers::kFlatColor, initialLayoutTransition);

    TraceRays(commandBuffer, imageIndex);

    const ImageLayoutTransition finalLayoutTransition{
        vk::ImageLayout::eGeneral, Renderer::kFinalLayout,
        PipelineBarrier{
            SyncScope::kRayTracingShaderWrite,
            SyncScope::kColorAttachmentWrite
        }
    };
    ImageHelpers::TransitImageLayout(commandBuffer,
            VulkanContext::swapchain->GetImages()[imageIndex],
            ImageHelpers::kFlatColor, finalLayoutTransition);
}

void RayTracer::OnResize(const vk::Extent2D &)
{
    SetupRenderTargets();
}

void RayTracer::SetupRenderObjects()
{
    scene.ForEachNode([this](Node &node)
        {
            for (const auto &renderObject : node.renderObjects)
            {
                const vk::AccelerationStructureNV blas
                        = VulkanContext::accelerationStructureManager->GenerateBlas(*renderObject);

                const RenderObjectUniforms uniforms{
                    blas, node.transform
                };

                renderObjects.emplace(renderObject.get(), uniforms);
            }
        });
}

void RayTracer::SetupRenderTargets()
{
    const DescriptorSetDescription description{
        { vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eRaygenNV },
    };

    renderTargetLayout = VulkanContext::descriptorPool->CreateDescriptorSetLayout(description);
    renderTargets = VulkanContext::descriptorPool->AllocateDescriptorSets(renderTargetLayout,
            static_cast<uint32_t>(VulkanContext::swapchain->GetImageViews().size()));

    for (uint32_t i = 0; i < renderTargets.size(); ++i)
    {
        const vk::DescriptorImageInfo imageInfo{
            nullptr, VulkanContext::swapchain->GetImageViews()[i], vk::ImageLayout::eGeneral
        };

        const DescriptorSetData descriptorSetData{
            DescriptorData{ vk::DescriptorType::eStorageImage, imageInfo }
        };

        VulkanContext::descriptorPool->UpdateDescriptorSet(renderTargets[i], descriptorSetData, 0);
    }
}

void RayTracer::SetupGlobalUniforms()
{
    const DescriptorSetDescription description{
        { vk::DescriptorType::eAccelerationStructureNV, vk::ShaderStageFlagBits::eRaygenNV },
        { vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eRaygenNV }
    };

    globalLayout = VulkanContext::descriptorPool->CreateDescriptorSetLayout(description);
    globalUniforms.descriptorSet = VulkanContext::descriptorPool->AllocateDescriptorSet(globalLayout);
    globalUniforms.tlas = VulkanContext::accelerationStructureManager->GenerateTlas(CollectGeometryInstances());
    globalUniforms.cameraBuffer = BufferHelpers::CreateUniformBuffer(sizeof(CameraData));

    const vk::WriteDescriptorSetAccelerationStructureNV accelerationStructureInfo{
        1, &globalUniforms.tlas
    };

    const vk::DescriptorBufferInfo bufferInfo{
        globalUniforms.cameraBuffer, 0, sizeof(CameraData)
    };

    const DescriptorSetData descriptorSetData{
        DescriptorData{ vk::DescriptorType::eAccelerationStructureNV, accelerationStructureInfo },
        DescriptorData{ vk::DescriptorType::eUniformBuffer, bufferInfo }
    };

    VulkanContext::descriptorPool->UpdateDescriptorSet(globalUniforms.descriptorSet, descriptorSetData, 0);
}

std::vector<GeometryInstance> RayTracer::CollectGeometryInstances()
{
    std::vector<GeometryInstance> geometryInstances;
    geometryInstances.reserve(renderObjects.size());

    for (const auto &[renderObject, uniforms] : renderObjects)
    {
        const GeometryInstance geometryInstance{
            uniforms.blas, uniforms.transform
        };

        geometryInstances.push_back(geometryInstance);
    }

    return geometryInstances;
}

void RayTracer::TraceRays(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const std::vector<vk::DescriptorSet> descriptorSets{
        renderTargets[imageIndex], globalUniforms.descriptorSet
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingNV, rayTracingPipeline->Get());
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingNV,
            rayTracingPipeline->GetLayout(), 0, descriptorSets, {});

    const ShaderBindingTable &shaderBindingTable = rayTracingPipeline->GetShaderBindingTable();
    const auto &[buffer, raygenOffset, missOffset, hitOffset, stride] = shaderBindingTable;

    const vk::Extent2D &extent = VulkanContext::swapchain->GetExtent();

    commandBuffer.traceRaysNV(buffer, raygenOffset,
            buffer, missOffset, stride,
            buffer, hitOffset, stride,
            nullptr, 0, 0,
            extent.width, extent.height, 1);
}
