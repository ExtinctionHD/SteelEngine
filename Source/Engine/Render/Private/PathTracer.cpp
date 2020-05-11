#include "Engine/Render/PathTracer.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCache.hpp"
#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Camera.hpp"

#include "Shaders/PathTracing/PathTracing.h"
#include "Shaders/Common/Common.h"

namespace SPathTracer
{
    constexpr LightingData kLighting{
        glm::vec4(1.0f, -1.0f, -0.5f, 0.0f),
        glm::vec4(1.0f, 1.0f, 1.0f, 4.0f)
    };

    const Filepath kEnvironmentPath("~/Assets/Textures/SunnyHills.hdr");

    constexpr vk::Extent2D kEnvironmentExtent(2048, 2048);

    std::unique_ptr<RayTracingPipeline> CreateRayTracingPipeline(
            const std::vector<vk::DescriptorSetLayout> &layouts)
    {
        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderCache->CreateShaderModule(
                    vk::ShaderStageFlagBits::eRaygenNV,
                    Filepath("~/Shaders/PathTracing/RayGen.rgen")),
            VulkanContext::shaderCache->CreateShaderModule(
                    vk::ShaderStageFlagBits::eMissNV,
                    Filepath("~/Shaders/PathTracing/Material.rmiss")),
            VulkanContext::shaderCache->CreateShaderModule(
                    vk::ShaderStageFlagBits::eMissNV,
                    Filepath("~/Shaders/PathTracing/Environment.rmiss")),
            VulkanContext::shaderCache->CreateShaderModule(
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

    template <class T>
    IndexedDescriptor CreateIndexedDescriptor(const T &info, vk::DescriptorType descriptorType)
    {
        const uint32_t descriptorCount = static_cast<uint32_t>(info.size());

        const DescriptorDescription description{
            descriptorType, descriptorCount,
            vk::ShaderStageFlagBits::eClosestHitNV,
            vk::DescriptorBindingFlagBits::eVariableDescriptorCount
        };

        const vk::DescriptorSetLayout layout
                = VulkanContext::descriptorPool->CreateDescriptorSetLayout({ description });

        const vk::DescriptorSet descriptorSet
                = VulkanContext::descriptorPool->AllocateDescriptorSets({ layout }, { descriptorCount }).front();

        const DescriptorData descriptorData{ descriptorType, info };

        VulkanContext::descriptorPool->UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return IndexedDescriptor{ layout, descriptorSet };
    }
}

PathTracer::PathTracer(Scene *scene_, Camera *camera_)
    : scene(scene_)
    , camera(camera_)
{
    scene->ForEachRenderObject(MakeFunction(&PathTracer::SetupRenderObject, this));

    SetupRenderTarget();
    SetupGlobalUniforms();
    SetupIndexedUniforms();

    const std::vector<vk::DescriptorSetLayout> layouts{
        renderTargetLayout, globalLayout,
        indexedUniforms.vertexBuffers.layout,
        indexedUniforms.indexBuffers.layout,
        indexedUniforms.materialBuffers.layout,
        indexedUniforms.baseColorTextures.layout,
        indexedUniforms.surfaceTextures.layout,
        indexedUniforms.normalTextures.layout
    };

    rayTracingPipeline = SPathTracer::CreateRayTracingPipeline(layouts);
}

void PathTracer::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    const CameraData cameraData{
        glm::inverse(camera->GetViewMatrix()),
        glm::inverse(camera->GetProjectionMatrix()),
        camera->GetDescription().zNear,
        camera->GetDescription().zFar
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

void PathTracer::OnResize(const vk::Extent2D &)
{
    ResetAccumulation();
    SetupRenderTarget();
}

void PathTracer::ResetAccumulation()
{
    accumulationIndex = 1;
}

void PathTracer::SetupRenderTarget()
{
    const std::vector<vk::ImageView> &imageViews = VulkanContext::swapchain->GetImageViews();

    const DescriptorSetDescription description{
        DescriptorDescription{
            vk::DescriptorType::eStorageImage, 1,
            vk::ShaderStageFlagBits::eRaygenNV,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            vk::DescriptorType::eStorageImage, 1,
            vk::ShaderStageFlagBits::eRaygenNV,
            vk::DescriptorBindingFlags()
        }
    };

    renderTargetLayout = VulkanContext::descriptorPool->CreateDescriptorSetLayout(description);
    renderTargets = VulkanContext::descriptorPool->AllocateDescriptorSets(
            Repeat(renderTargetLayout, imageViews.size()));

    for (uint32_t i = 0; i < renderTargets.size(); ++i)
    {
        const uint32_t prevI = i > 0 ? i - 1 : static_cast<uint32_t>(renderTargets.size()) - 1;

        const vk::DescriptorImageInfo prevRenderTarget(nullptr, imageViews[prevI], vk::ImageLayout::eGeneral);
        const vk::DescriptorImageInfo currentRenderTarget(nullptr, imageViews[i], vk::ImageLayout::eGeneral);

        const DescriptorSetData descriptorSetData{
            DescriptorData{
                vk::DescriptorType::eStorageImage,
                ImageInfo{ prevRenderTarget }
            },
            DescriptorData{
                vk::DescriptorType::eStorageImage,
                ImageInfo{ currentRenderTarget }
            },
        };

        VulkanContext::descriptorPool->UpdateDescriptorSet(renderTargets[i], descriptorSetData, 0);
    }
}

void PathTracer::SetupGlobalUniforms()
{
    const DescriptorSetDescription description{
        DescriptorDescription{
            vk::DescriptorType::eAccelerationStructureNV, 1,
            vk::ShaderStageFlagBits::eRaygenNV | vk::ShaderStageFlagBits::eClosestHitNV,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eRaygenNV,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eClosestHitNV,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            vk::DescriptorType::eCombinedImageSampler, 1,
            vk::ShaderStageFlagBits::eClosestHitNV | vk::ShaderStageFlagBits::eMissNV,
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
    globalUniforms.lightingBuffer = BufferHelpers::CreateUniformBuffer(sizeof(LightingData));

    globalUniforms.environmentMap = VulkanContext::textureCache->CreateCubeTexture(
            VulkanContext::textureCache->GetTexture(SPathTracer::kEnvironmentPath, SamplerDescription{}),
            SPathTracer::kEnvironmentExtent, SamplerDescription{});

    const DescriptorSetData descriptorSetData{
        DescriptorData{
            vk::DescriptorType::eAccelerationStructureNV,
            DescriptorHelpers::GetInfo(globalUniforms.tlas)
        },
        DescriptorData{
            vk::DescriptorType::eUniformBuffer,
            DescriptorHelpers::GetInfo(globalUniforms.cameraBuffer)
        },
        DescriptorData{
            vk::DescriptorType::eUniformBuffer,
            DescriptorHelpers::GetInfo(globalUniforms.lightingBuffer)
        },
        DescriptorData{
            vk::DescriptorType::eCombinedImageSampler,
            DescriptorHelpers::GetInfo(globalUniforms.environmentMap)
        }
    };

    VulkanContext::descriptorPool->UpdateDescriptorSet(globalUniforms.descriptorSet, descriptorSetData, 0);

    VulkanContext::device->ExecuteOneTimeCommands(std::bind(&BufferHelpers::UpdateBuffer, std::placeholders::_1,
            globalUniforms.lightingBuffer, GetByteView(SPathTracer::kLighting), SyncScope::kRayTracingShaderRead));
}

void PathTracer::SetupIndexedUniforms()
{
    BufferInfo vertexBuffersInfo;
    BufferInfo indexBuffersInfo;
    BufferInfo materialBuffersInfo;

    ImageInfo baseColorTexturesInfo;
    ImageInfo surfaceTexturesInfo;
    ImageInfo normalTexturesInfo;

    for (const auto &[renderObject, entry] : renderObjects)
    {
        vertexBuffersInfo.emplace_back(entry.vertexBuffer, 0, VK_WHOLE_SIZE);
        indexBuffersInfo.emplace_back(entry.indexBuffer, 0, VK_WHOLE_SIZE);
        materialBuffersInfo.emplace_back(entry.materialBuffer, 0, VK_WHOLE_SIZE);

        const Texture &baseColorTexture = renderObject->GetMaterial().baseColorTexture;
        const Texture &surfaceTexture = renderObject->GetMaterial().surfaceTexture;
        const Texture &normalTexture = renderObject->GetMaterial().normalTexture;

        baseColorTexturesInfo.emplace_back(baseColorTexture.sampler, baseColorTexture.view, Texture::kLayout);
        surfaceTexturesInfo.emplace_back(surfaceTexture.sampler, surfaceTexture.view, Texture::kLayout);
        normalTexturesInfo.emplace_back(normalTexture.sampler, normalTexture.view, Texture::kLayout);
    }

    indexedUniforms.vertexBuffers = SPathTracer::CreateIndexedDescriptor(
            vertexBuffersInfo, vk::DescriptorType::eStorageBuffer);
    indexedUniforms.indexBuffers = SPathTracer::CreateIndexedDescriptor(
            indexBuffersInfo, vk::DescriptorType::eStorageBuffer);
    indexedUniforms.materialBuffers = SPathTracer::CreateIndexedDescriptor(
            materialBuffersInfo, vk::DescriptorType::eUniformBuffer);
    indexedUniforms.baseColorTextures = SPathTracer::CreateIndexedDescriptor(
            baseColorTexturesInfo, vk::DescriptorType::eCombinedImageSampler);
    indexedUniforms.surfaceTextures = SPathTracer::CreateIndexedDescriptor(
            surfaceTexturesInfo, vk::DescriptorType::eCombinedImageSampler);
    indexedUniforms.normalTextures = SPathTracer::CreateIndexedDescriptor(
            normalTexturesInfo, vk::DescriptorType::eCombinedImageSampler);
}

void PathTracer::SetupRenderObject(const RenderObject &renderObject, const glm::mat4 &transform)
{
    const std::vector<VertexData> vertices = SPathTracer::ConvertVertices(renderObject.GetVertices());
    const std::vector<uint32_t> &indices = renderObject.GetIndices();
    const Material &material = renderObject.GetMaterial();

    const vk::Buffer vertexBuffer = BufferHelpers::CreateStorageBuffer(vertices.size() * sizeof(VertexData));
    const vk::Buffer indexBuffer = BufferHelpers::CreateStorageBuffer(indices.size() * sizeof(uint32_t));
    const vk::Buffer materialBuffer = BufferHelpers::CreateUniformBuffer(sizeof(MaterialFactors));

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            BufferHelpers::UpdateBuffer(commandBuffer, vertexBuffer,
                    GetByteView(vertices), SyncScope::kAccelerationStructureBuild);

            BufferHelpers::UpdateBuffer(commandBuffer, indexBuffer,
                    GetByteView(indices), SyncScope::kAccelerationStructureBuild);

            BufferHelpers::UpdateBuffer(commandBuffer, materialBuffer,
                    GetByteView(material.factors), SyncScope::kRayTracingShaderRead);
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
        vertexBuffer,
        indexBuffer,
        materialBuffer,
        GeometryInstance{
            blas, transform
        }
    };

    renderObjects.emplace(&renderObject, entry);
}

void PathTracer::TraceRays(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    const std::vector<vk::DescriptorSet> descriptorSets{
        renderTargets[imageIndex], globalUniforms.descriptorSet,
        indexedUniforms.vertexBuffers.descriptorSet,
        indexedUniforms.indexBuffers.descriptorSet,
        indexedUniforms.materialBuffers.descriptorSet,
        indexedUniforms.baseColorTextures.descriptorSet,
        indexedUniforms.surfaceTextures.descriptorSet,
        indexedUniforms.normalTextures.descriptorSet
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
            extent.width, extent.height, 1);
}
