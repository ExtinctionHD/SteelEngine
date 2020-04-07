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

    std::vector<VertexData> ConvertVertices(const std::vector<Vertex> &vertices)
    {
        std::vector<VertexData> convertedVertices;
        convertedVertices.reserve(vertices.size());

        for (const auto &vertex : vertices)
        {
            const VertexData convertedVertex{
                glm::vec4(vertex.position, 1.0f),
                glm::vec4(vertex.normal, 0.0f),
                glm::vec4(vertex.tangent, 0.0f),
                glm::vec4(vertex.texCoord, 0.0f, 0.0f)
            };

            convertedVertices.push_back(convertedVertex);
        }

        return convertedVertices;
    }

    std::pair<vk::DescriptorSetLayout, vk::DescriptorSet> CreateBuffersUniform(BuffersInfo buffersInfo)
    {
        const uint32_t descriptorCount = static_cast<uint32_t>(buffersInfo.size());

        const DescriptorDescription description{
            vk::DescriptorType::eStorageBuffer, descriptorCount,
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
            vk::DescriptorType::eCombinedImageSampler, descriptorCount,
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
    scene.ForEachRenderObject(MakeFunction(&RayTracer::SetupRenderObject, this));

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

    BufferHelpers::UpdateBuffer(commandBuffer, globalUniforms.cameraBuffer,
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
    const std::vector<vk::ImageView> &imageViews = VulkanContext::swapchain->GetImageViews();

    const DescriptorDescription description{
        vk::DescriptorType::eStorageImage, 1,
        vk::ShaderStageFlagBits::eRaygenNV,
        vk::DescriptorBindingFlags()
    };

    renderTargetLayout = VulkanContext::descriptorPool->CreateDescriptorSetLayout({ description });
    renderTargets = VulkanContext::descriptorPool->AllocateDescriptorSets(
            Repeat(renderTargetLayout, imageViews.size()));

    for (uint32_t i = 0; i < renderTargets.size(); ++i)
    {
        const vk::DescriptorImageInfo imageInfo(nullptr, imageViews[i], vk::ImageLayout::eGeneral);

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
            vk::DescriptorType::eAccelerationStructureNV, 1,
            vk::ShaderStageFlagBits::eRaygenNV,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eRaygenNV,
            vk::DescriptorBindingFlags()
        }
    };

    std::vector<GeometryInstance> geometryInstances;
    for (const auto &[renderObject, entry] : renderObjects)
    {
        geometryInstances.push_back(entry.geometryInstance);
    }

    globalLayout = VulkanContext::descriptorPool->CreateDescriptorSetLayout(description);
    globalUniforms.descriptorSet = VulkanContext::descriptorPool->AllocateDescriptorSets({ globalLayout }).front();
    globalUniforms.tlas = VulkanContext::accelerationStructureManager->GenerateTlas(geometryInstances);
    globalUniforms.cameraBuffer = BufferHelpers::CreateUniformBuffer(sizeof(CameraData));

    const vk::WriteDescriptorSetAccelerationStructureNV accelerationStructuresInfo{
        1, &globalUniforms.tlas
    };

    const DescriptorSetData descriptorSetData{
        DescriptorData{
            vk::DescriptorType::eAccelerationStructureNV,
            accelerationStructuresInfo
        },
        DescriptorData{
            vk::DescriptorType::eUniformBuffer,
            BuffersInfo{ BufferHelpers::GetInfo(globalUniforms.cameraBuffer) }
        }
    };

    VulkanContext::descriptorPool->UpdateDescriptorSet(globalUniforms.descriptorSet, descriptorSetData, 0);
}

void RayTracer::SetupIndexedUniforms()
{
    BuffersInfo vertexBuffersInfo;
    BuffersInfo indexBuffersInfo;
    ImagesInfo baseColorTexturesInfo;

    for (const auto &[renderObject, entry] : renderObjects)
    {
        vertexBuffersInfo.push_back(BufferHelpers::GetInfo(entry.vertexBuffer));
        indexBuffersInfo.push_back(BufferHelpers::GetInfo(entry.indexBuffer));

        const Texture &baseColorTexture = renderObject->GetMaterial().baseColorTexture;
        baseColorTexturesInfo.push_back(TextureHelpers::GetInfo(baseColorTexture));
    }

    std::tie(indexedUniforms.vertexBuffers.layout, indexedUniforms.vertexBuffers.descriptorSet)
            = SRayTracer::CreateBuffersUniform(vertexBuffersInfo);

    std::tie(indexedUniforms.indexBuffers.layout, indexedUniforms.indexBuffers.descriptorSet)
            = SRayTracer::CreateBuffersUniform(indexBuffersInfo);

    std::tie(indexedUniforms.baseColorTextures.layout, indexedUniforms.baseColorTextures.descriptorSet)
            = SRayTracer::CreateTexturesUniform(baseColorTexturesInfo);
}

void RayTracer::SetupRenderObject(const RenderObject &renderObject, const glm::mat4 &transform)
{
    const std::vector<VertexData> vertices = SRayTracer::ConvertVertices(renderObject.GetVertices());
    const std::vector<uint32_t> indices = renderObject.GetIndices();

    const vk::Buffer vertexBuffer = BufferHelpers::CreateStorageBuffer(vertices.size() * sizeof(VertexData));
    const vk::Buffer indexBuffer = BufferHelpers::CreateStorageBuffer(indices.size() * sizeof(uint32_t));

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            BufferHelpers::UpdateBuffer(commandBuffer, vertexBuffer,
                    GetByteView(vertices), SyncScope::kAccelerationStructureBuild);

            BufferHelpers::UpdateBuffer(commandBuffer, indexBuffer,
                    GetByteView(indices), SyncScope::kAccelerationStructureBuild);
        });

    const GeometryVertices geometryVertices{
        vertexBuffer,
        RenderObject::KVertexFormat,
        renderObject.GetVertexCount(),
        sizeof(VertexData)
    };

    const GeometryIndices geometryIndices{
        indexBuffer,
        RenderObject::kIndexType,
        renderObject.GetIndexCount()
    };

    const vk::AccelerationStructureNV blas
            = VulkanContext::accelerationStructureManager->GenerateBlas(
                    geometryVertices, geometryIndices);

    const RenderObjectEntry entry{
        vertexBuffer, indexBuffer,
        GeometryInstance{
            blas, transform
        }
    };

    renderObjects.emplace(&renderObject, entry);
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
