#include "Engine/Render/RayTracer.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCache.hpp"
#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Camera.hpp"

namespace SRayTracer
{
    std::unique_ptr<RayTracingPipeline> CreateRayTracingPipeline(
            const std::vector<vk::DescriptorSetLayout> &layouts)
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
            shaderModules, shaderGroups, layouts, {}
        };

        return RayTracingPipeline::Create(description);
    }

    std::vector<GeometryInstance> GetGeometryInstances(const Scene &scene)
    {
        std::vector<GeometryInstance> geometryInstances;

        scene.ForEachNode([&geometryInstances](const Node &node)
            {
                for (const auto &renderObject : node.renderObjects)
                {
                    const vk::AccelerationStructureNV blas
                            = VulkanContext::accelerationStructureManager->GenerateBlas(*renderObject);

                    const GeometryInstance geometryInstance{ blas, node.transform };

                    geometryInstances.push_back(geometryInstance);
                }
            });

        return geometryInstances;
    }

    std::pair<vk::DescriptorSetLayout, vk::DescriptorSet> CreateBuffersUniform(BuffersInfo buffersInfo)
    {
        const uint32_t descriptorCount = static_cast<uint32_t>(buffersInfo.size());

        const DescriptorDescription description{
            vk::DescriptorType::eStorageBuffer,
            vk::ShaderStageFlagBits::eClosestHitNV,
            vk::DescriptorBindingFlagBits::eVariableDescriptorCount
        };

        const vk::DescriptorSetLayout layout
                = VulkanContext::descriptorPool->CreateDescriptorSetLayout({ description });

        const vk::DescriptorSet descriptorSet
                = VulkanContext::descriptorPool->AllocateDescriptorSets({ layout }, { descriptorCount }).front();

        const DescriptorData descriptorData{ vk::DescriptorType::eStorageBuffer, buffersInfo };

        VulkanContext::descriptorPool->UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return std::make_pair(layout, descriptorSet);
    }

    std::pair<vk::DescriptorSetLayout, vk::DescriptorSet> CreateTexturesUniform(ImagesInfo imageInfos)
    {
        const uint32_t descriptorCount = static_cast<uint32_t>(imageInfos.size());

        const DescriptorDescription description{
            vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eClosestHitNV,
            vk::DescriptorBindingFlagBits::eVariableDescriptorCount
        };

        const vk::DescriptorSetLayout layout
                = VulkanContext::descriptorPool->CreateDescriptorSetLayout({ description });

        const vk::DescriptorSet descriptorSet
                = VulkanContext::descriptorPool->AllocateDescriptorSets({ layout }, { descriptorCount }).front();

        const DescriptorData descriptorData{ vk::DescriptorType::eCombinedImageSampler, imageInfos };

        VulkanContext::descriptorPool->UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return std::make_pair(layout, descriptorSet);
    }
}

RayTracer::RayTracer(Scene &scene_, Camera &camera_)
    : scene(scene_)
    , camera(camera_)
{
    SetupRenderTarget();
    SetupGlobalUniforms();
    SetupIndexedUniforms();

    const std::vector<vk::DescriptorSetLayout> layouts{
        renderTargetLayout, globalLayout,
        indexedUniforms.vertexBuffers.layout,
        indexedUniforms.indexBuffers.layout,
        indexedUniforms.baseColorTextures.layout
    };

    rayTracingPipeline = SRayTracer::CreateRayTracingPipeline(layouts);
}

void RayTracer::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    const CameraData cameraData{
        glm::inverse(camera.GetViewMatrix()),
        glm::inverse(camera.GetProjectionMatrix()),
        camera.GetDescription().zNear,
        camera.GetDescription().zFar
    };

    BufferHelpers::UpdateUniformBuffer(commandBuffer, globalUniforms.cameraBuffer,
            GetByteView(cameraData), SyncScope::kRayTracingShaderRead);

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
    SetupRenderTarget();
}

void RayTracer::SetupRenderTarget()
{
    const DescriptorDescription description{
        vk::DescriptorType::eStorageImage,
        vk::ShaderStageFlagBits::eRaygenNV,
        vk::DescriptorBindingFlags()
    };

    renderTargetLayout = VulkanContext::descriptorPool->CreateDescriptorSetLayout({ description });

    const std::vector<vk::DescriptorSetLayout> layouts(VulkanContext::swapchain->GetImageViews().size(),
            renderTargetLayout);
    renderTargets = VulkanContext::descriptorPool->AllocateDescriptorSets(layouts);

    for (uint32_t i = 0; i < renderTargets.size(); ++i)
    {
        const vk::DescriptorImageInfo imageInfo{
            nullptr, VulkanContext::swapchain->GetImageViews()[i], vk::ImageLayout::eGeneral
        };

        const DescriptorData descriptorData{
            vk::DescriptorType::eStorageImage, ImagesInfo{ imageInfo }
        };

        VulkanContext::descriptorPool->UpdateDescriptorSet(renderTargets[i], { descriptorData }, 0);
    }
}

void RayTracer::SetupGlobalUniforms()
{
    const DescriptorSetDescription description{
        DescriptorDescription{
            vk::DescriptorType::eAccelerationStructureNV,
            vk::ShaderStageFlagBits::eRaygenNV,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eRaygenNV,
            vk::DescriptorBindingFlags()
        }
    };

    const std::vector<GeometryInstance> geometryInstances = SRayTracer::GetGeometryInstances(scene);

    globalLayout = VulkanContext::descriptorPool->CreateDescriptorSetLayout(description);
    globalUniforms.descriptorSet = VulkanContext::descriptorPool->AllocateDescriptorSets({ globalLayout }).front();
    globalUniforms.tlas = VulkanContext::accelerationStructureManager->GenerateTlas(geometryInstances);
    globalUniforms.cameraBuffer = BufferHelpers::CreateUniformBuffer(sizeof(CameraData));

    const vk::WriteDescriptorSetAccelerationStructureNV accelerationStructuresInfo{
        1, &globalUniforms.tlas
    };

    const vk::DescriptorBufferInfo bufferInfo{
        globalUniforms.cameraBuffer, 0, sizeof(CameraData)
    };

    const DescriptorSetData descriptorSetData{
        DescriptorData{ vk::DescriptorType::eAccelerationStructureNV, accelerationStructuresInfo },
        DescriptorData{ vk::DescriptorType::eUniformBuffer, BuffersInfo{ bufferInfo } }
    };

    VulkanContext::descriptorPool->UpdateDescriptorSet(globalUniforms.descriptorSet, descriptorSetData, 0);
}

void RayTracer::SetupIndexedUniforms()
{
    BuffersInfo vertexBuffersInfo;
    BuffersInfo indexBuffersInfo;
    ImagesInfo baseColorTexturesInfo;

    scene.ForEachNode([&vertexBuffersInfo, &indexBuffersInfo, &baseColorTexturesInfo](const Node &node)
        {
            for (const auto &renderObject : node.renderObjects)
            {
                vertexBuffersInfo.emplace_back(renderObject->GetVertexBuffer(), 0, VK_WHOLE_SIZE);
                indexBuffersInfo.emplace_back(renderObject->GetIndexBuffer(), 0, VK_WHOLE_SIZE);

                const Texture &baseColorTexture = renderObject->GetMaterial().baseColorTexture;
                baseColorTexturesInfo.emplace_back(baseColorTexture.sampler, baseColorTexture.view, vk::ImageLayout::eShaderReadOnlyOptimal);
            }
        });

    std::tie(indexedUniforms.vertexBuffers.layout, indexedUniforms.vertexBuffers.descriptorSet)
            = SRayTracer::CreateBuffersUniform(vertexBuffersInfo);

    std::tie(indexedUniforms.indexBuffers.layout, indexedUniforms.indexBuffers.descriptorSet)
            = SRayTracer::CreateBuffersUniform(indexBuffersInfo);

    std::tie(indexedUniforms.baseColorTextures.layout, indexedUniforms.baseColorTextures.descriptorSet)
            = SRayTracer::CreateTexturesUniform(baseColorTexturesInfo);

}

void RayTracer::TraceRays(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const std::vector<vk::DescriptorSet> descriptorSets{
        renderTargets[imageIndex], globalUniforms.descriptorSet,
        indexedUniforms.vertexBuffers.descriptorSet,
        indexedUniforms.indexBuffers.descriptorSet,
        indexedUniforms.baseColorTextures.descriptorSet
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
