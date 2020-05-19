#include "Engine/System/PathTracingSystem.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCache.hpp"
#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Engine.hpp"

#include "Shaders/Common/Common.h"
#include "Shaders/PathTracing/PathTracing.h"

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

PathTracingSystem::PathTracingSystem(Scene *scene_, Camera *camera_)
    : scene(scene_)
    , camera(camera_)
{
    scene->ForEachRenderObject(MakeFunction(this, &PathTracingSystem::SetupRenderObject));

    SetupRenderTargets();
    SetupAccumulationTarget();
    SetupGlobalUniforms();
    SetupIndexedUniforms();

    const std::vector<vk::DescriptorSetLayout> rayTracingLayouts{
        renderTargets.layout,
        accumulationTarget.layout,
        globalLayout,
        indexedUniforms.vertexBuffers.layout,
        indexedUniforms.indexBuffers.layout,
        indexedUniforms.materialBuffers.layout,
        indexedUniforms.baseColorTextures.layout,
        indexedUniforms.surfaceTextures.layout,
        indexedUniforms.normalTextures.layout
    };

    rayTracingPipeline = SPathTracer::CreateRayTracingPipeline(rayTracingLayouts);

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &PathTracingSystem::HandleResizeEvent));

    Engine::AddEventHandler(EventType::eCameraUpdate,
            MakeFunction(this, &PathTracingSystem::ResetAccumulation));
}

PathTracingSystem::~PathTracingSystem()
{
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(renderTargets.layout);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(accumulationTarget.layout);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(globalLayout);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(indexedUniforms.vertexBuffers.layout);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(indexedUniforms.indexBuffers.layout);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(indexedUniforms.materialBuffers.layout);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(indexedUniforms.baseColorTextures.layout);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(indexedUniforms.surfaceTextures.layout);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(indexedUniforms.normalTextures.layout);
}

void PathTracingSystem::Process(float) {}

void PathTracingSystem::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    const CameraData cameraData{
        glm::inverse(camera->GetViewMatrix()),
        glm::inverse(camera->GetProjectionMatrix()),
        camera->GetDescription().zNear,
        camera->GetDescription().zFar
    };

    BufferHelpers::UpdateBuffer(commandBuffer, globalUniforms.cameraBuffer,
            GetByteView(cameraData), SyncScope::kRayTracingShaderRead);

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
        vk::DescriptorType::eStorageImage, 1,
        vk::ShaderStageFlagBits::eRaygenNV,
        vk::DescriptorBindingFlags()
    };

    DescriptorPool &descriptorPool = *VulkanContext::descriptorPool;
    renderTargets.layout = descriptorPool.CreateDescriptorSetLayout({ descriptorDescription });
    renderTargets.descriptorSets = descriptorPool.AllocateDescriptorSets(
            Repeat(renderTargets.layout, swapchainImageViews.size()));

    for (size_t i = 0; i < swapchainImageViews.size(); ++i)
    {
        const DescriptorData descriptorData{
            vk::DescriptorType::eStorageImage,
            DescriptorHelpers::GetInfo(swapchainImageViews[i])
        };

        descriptorPool.UpdateDescriptorSet(renderTargets.descriptorSets[i], { descriptorData }, 0);
    }
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
        vk::DescriptorType::eStorageImage, 1,
        vk::ShaderStageFlagBits::eRaygenNV,
        vk::DescriptorBindingFlags()
    };

    const DescriptorData descriptorData{
        vk::DescriptorType::eStorageImage,
        DescriptorHelpers::GetInfo(accumulationTarget.view)
    };

    DescriptorPool &descriptorPool = *VulkanContext::descriptorPool;
    accumulationTarget.layout = descriptorPool.CreateDescriptorSetLayout({ descriptorDescription });
    accumulationTarget.descriptorSet = descriptorPool.AllocateDescriptorSets({ accumulationTarget.layout }).front();
    descriptorPool.UpdateDescriptorSet(accumulationTarget.descriptorSet, { descriptorData }, 0);

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

void PathTracingSystem::SetupGlobalUniforms()
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

    const Texture panoramaTexture = VulkanContext::textureCache->CreateTexture(SPathTracer::kEnvironmentPath);
    globalUniforms.environmentMap = VulkanContext::textureCache->CreateCubeTexture(
            panoramaTexture, SPathTracer::kEnvironmentExtent);
    VulkanContext::textureCache->DestroyTexture(panoramaTexture);

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
            DescriptorHelpers::GetInfo(globalUniforms.environmentMap.sampler,
                    globalUniforms.environmentMap.view)
        }
    };

    VulkanContext::descriptorPool->UpdateDescriptorSet(globalUniforms.descriptorSet, descriptorSetData, 0);

    VulkanContext::device->ExecuteOneTimeCommands(std::bind(&BufferHelpers::UpdateBuffer, std::placeholders::_1,
            globalUniforms.lightingBuffer, GetByteView(SPathTracer::kLighting), SyncScope::kRayTracingShaderRead));
}

void PathTracingSystem::SetupIndexedUniforms()
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

        constexpr vk::ImageLayout textureLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        baseColorTexturesInfo.emplace_back(baseColorTexture.sampler, baseColorTexture.view, textureLayout);
        surfaceTexturesInfo.emplace_back(surfaceTexture.sampler, surfaceTexture.view, textureLayout);
        normalTexturesInfo.emplace_back(normalTexture.sampler, normalTexture.view, textureLayout);
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

void PathTracingSystem::SetupRenderObject(const RenderObject &renderObject, const glm::mat4 &transform)
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

void PathTracingSystem::TraceRays(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    const std::vector<vk::DescriptorSet> descriptorSets{
        renderTargets.descriptorSets[imageIndex],
        accumulationTarget.descriptorSet,
        globalUniforms.descriptorSet,
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

void PathTracingSystem::HandleResizeEvent(const vk::Extent2D &extent)
{
    if (extent.width != 0 && extent.height != 0)
    {
        ResetAccumulation();

        VulkanContext::descriptorPool->FreeDescriptorSets(renderTargets.descriptorSets);
        VulkanContext::descriptorPool->DestroyDescriptorSetLayout(renderTargets.layout);

        VulkanContext::descriptorPool->FreeDescriptorSets({ accumulationTarget.descriptorSet });
        VulkanContext::descriptorPool->DestroyDescriptorSetLayout(accumulationTarget.layout);
        VulkanContext::imageManager->DestroyImage(accumulationTarget.image);

        SetupRenderTargets();
        SetupAccumulationTarget();
    }
}

void PathTracingSystem::ResetAccumulation()
{
    accumulationIndex = 0;
}
