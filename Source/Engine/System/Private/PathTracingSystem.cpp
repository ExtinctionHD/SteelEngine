#include "Engine/System/PathTracingSystem.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"
#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Engine.hpp"

#include "Shaders/Common/Common.h"
#include "Shaders/PathTracing/PathTracing.h"

namespace SPathTracingSystem
{
    constexpr LightingData kLighting{
        glm::vec4(1.0f, -1.0f, -0.5f, 0.0f),
        glm::vec4(1.0f, 1.0f, 1.0f, 4.0f)
    };

    const Filepath kEnvironmentPath("~/Assets/Textures/VizPeople/Sunset2.hdr");

    constexpr vk::Extent2D kEnvironmentExtent(2048, 2048);

    vk::Buffer CreateUniformBuffer(vk::DeviceSize size)
    {
        const vk::BufferUsageFlags usage
                = vk::BufferUsageFlagBits::eUniformBuffer
                | vk::BufferUsageFlagBits::eTransferDst;

        return BufferHelpers::CreateDeviceLocalBuffer(size, usage);
    }

    std::unique_ptr<RayTracingPipeline> CreateRayTracingPipeline(
            const std::vector<vk::DescriptorSetLayout> &layouts)
    {
        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eRaygenKHR,
                    Filepath("~/Shaders/PathTracing/RayGen.rgen")),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eMissKHR,
                    Filepath("~/Shaders/PathTracing/Material.rmiss")),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eMissKHR,
                    Filepath("~/Shaders/PathTracing/Environment.rmiss")),
            VulkanContext::shaderManager->CreateShaderModule(
                    vk::ShaderStageFlagBits::eClosestHitKHR,
                    Filepath("~/Shaders/PathTracing/Material.rchit"))
        };

        const std::vector<RayTracingPipeline::ShaderGroup> shaderGroups{
            RayTracingPipeline::ShaderGroup{
                vk::RayTracingShaderGroupTypeKHR::eGeneral,
                0, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR
            },
            RayTracingPipeline::ShaderGroup{
                vk::RayTracingShaderGroupTypeKHR::eGeneral,
                1, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR
            },
            RayTracingPipeline::ShaderGroup{
                vk::RayTracingShaderGroupTypeKHR::eGeneral,
                2, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR
            },
            RayTracingPipeline::ShaderGroup{
                vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
                VK_SHADER_UNUSED_KHR, 3, VK_SHADER_UNUSED_KHR
            },
        };

        const vk::PushConstantRange pushConstant(vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(uint32_t));

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
    DescriptorSet CreateIndexedDescriptor(const DescriptorData &descriptorData)
    {
        const uint32_t descriptorCount = static_cast<uint32_t>(std::get<T>(descriptorData.descriptorInfo).size());

        const DescriptorDescription description{
            descriptorData.type, descriptorCount,
            vk::ShaderStageFlagBits::eClosestHitKHR,
            vk::DescriptorBindingFlagBits::eVariableDescriptorCount
        };

        return DescriptorHelpers::CreateDescriptorSet({ description }, { descriptorData });
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

    rayTracingPipeline = SPathTracingSystem::CreateRayTracingPipeline(rayTracingLayouts);

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &PathTracingSystem::HandleResizeEvent));

    Engine::AddEventHandler(EventType::eCameraUpdate,
            MakeFunction(this, &PathTracingSystem::ResetAccumulation));
}

PathTracingSystem::~PathTracingSystem()
{
    DescriptorHelpers::DestroyMultiDescriptorSet(renderTargets.multiDescriptor);
    DescriptorHelpers::DestroyDescriptorSet(accumulationTarget.descriptor);
    DescriptorHelpers::DestroyDescriptorSet(globalUniforms.descriptorSet);
    DescriptorHelpers::DestroyDescriptorSet(indexedUniforms.vertexBuffersDescriptor);
    DescriptorHelpers::DestroyDescriptorSet(indexedUniforms.indexBuffersDescriptor);
    DescriptorHelpers::DestroyDescriptorSet(indexedUniforms.materialBuffersDescriptor);
    DescriptorHelpers::DestroyDescriptorSet(indexedUniforms.baseColorTexturesDescriptor);
    DescriptorHelpers::DestroyDescriptorSet(indexedUniforms.surfaceTexturesDescriptor);
    DescriptorHelpers::DestroyDescriptorSet(indexedUniforms.normalTexturesDescriptor);
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
        vk::ShaderStageFlagBits::eRaygenKHR,
        vk::DescriptorBindingFlags()
    };

    std::vector<DescriptorSetData> multiDescriptorData;
    multiDescriptorData.reserve(swapchainImageViews.size());

    for (const auto &swapchainImageView : swapchainImageViews)
    {
        multiDescriptorData.push_back({ DescriptorHelpers::GetData(swapchainImageView) });
    }

    renderTargets.multiDescriptor = DescriptorHelpers::CreateMultiDescriptorSet({ descriptorDescription },
            multiDescriptorData);
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
        vk::ShaderStageFlagBits::eRaygenKHR,
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

void PathTracingSystem::SetupGlobalUniforms()
{
    auto &[tlas, cameraBuffer, lightingBuffer, environmentMap, descriptorSet] = globalUniforms;

    std::vector<GeometryInstanceData> geometryInstances;
    for (const auto &[renderObject, entry] : renderObjects)
    {
        geometryInstances.push_back(entry.geometryInstance);
    }

    tlas = VulkanContext::accelerationStructureManager->GenerateTlas(geometryInstances);
    cameraBuffer = SPathTracingSystem::CreateUniformBuffer(sizeof(CameraData));
    lightingBuffer = SPathTracingSystem::CreateUniformBuffer(sizeof(LightingData));

    VulkanContext::device->ExecuteOneTimeCommands([&lightingBuffer](vk::CommandBuffer commandBuffer)
        {
            BufferHelpers::UpdateBuffer(commandBuffer, lightingBuffer,
                    GetByteView(SPathTracingSystem::kLighting), SyncScope::kRayTracingShaderRead);
        });

    // TODO
    const Texture panoramaTexture = VulkanContext::textureManager->CreateColorTexture(glm::vec3(0.0f, 1.0f, 0.0f));
    environmentMap = VulkanContext::textureManager->CreateCubeTexture(panoramaTexture,
            SPathTracingSystem::kEnvironmentExtent);
    VulkanContext::textureManager->DestroyTexture(panoramaTexture);

    const DescriptorSetDescription descriptorSetDescription{
        DescriptorDescription{
            vk::DescriptorType::eAccelerationStructureKHR, 1,
            vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eRaygenKHR,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eClosestHitKHR,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            vk::DescriptorType::eCombinedImageSampler, 1,
            vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR,
            vk::DescriptorBindingFlags()
        }
    };

    const DescriptorSetData descriptorSetData{
        DescriptorHelpers::GetData(tlas),
        DescriptorHelpers::GetData(cameraBuffer),
        DescriptorHelpers::GetData(lightingBuffer),
        DescriptorHelpers::GetData(environmentMap.sampler, environmentMap.view)
    };

    descriptorSet = DescriptorHelpers::CreateDescriptorSet(descriptorSetDescription, descriptorSetData);
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

    indexedUniforms.vertexBuffersDescriptor = SPathTracingSystem::CreateIndexedDescriptor<BufferInfo>(
            DescriptorData{ vk::DescriptorType::eStorageBuffer, vertexBuffersInfo });
    indexedUniforms.indexBuffersDescriptor = SPathTracingSystem::CreateIndexedDescriptor<BufferInfo>(
            DescriptorData{ vk::DescriptorType::eStorageBuffer, indexBuffersInfo });
    indexedUniforms.materialBuffersDescriptor = SPathTracingSystem::CreateIndexedDescriptor<BufferInfo>(
            DescriptorData{ vk::DescriptorType::eUniformBuffer, materialBuffersInfo });

    indexedUniforms.baseColorTexturesDescriptor = SPathTracingSystem::CreateIndexedDescriptor<ImageInfo>(
            DescriptorData{ vk::DescriptorType::eCombinedImageSampler, baseColorTexturesInfo });
    indexedUniforms.surfaceTexturesDescriptor = SPathTracingSystem::CreateIndexedDescriptor<ImageInfo>(
            DescriptorData{ vk::DescriptorType::eCombinedImageSampler, surfaceTexturesInfo });
    indexedUniforms.normalTexturesDescriptor = SPathTracingSystem::CreateIndexedDescriptor<ImageInfo>(
            DescriptorData{ vk::DescriptorType::eCombinedImageSampler, normalTexturesInfo });
}

void PathTracingSystem::SetupRenderObject(const RenderObject &renderObject, const glm::mat4 &transform)
{
    const std::vector<VertexData> vertices = SPathTracingSystem::ConvertVertices(renderObject.GetVertices());
    const std::vector<uint32_t> &indices = renderObject.GetIndices();
    const Material &material = renderObject.GetMaterial();

    const vk::BufferUsageFlags geometryBufferUsage = vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress;

    const vk::Buffer vertexBuffer = BufferHelpers::CreateDeviceLocalBuffer(
            vertices.size() * sizeof(VertexData), geometryBufferUsage);

    const vk::Buffer indexBuffer = BufferHelpers::CreateDeviceLocalBuffer(
            indices.size() * sizeof(uint32_t), geometryBufferUsage);

    const vk::Buffer materialBuffer = SPathTracingSystem::CreateUniformBuffer(sizeof(MaterialFactors));

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            BufferHelpers::UpdateBuffer(commandBuffer, vertexBuffer,
                    GetByteView(vertices), SyncScope::kAccelerationStructureBuild);

            BufferHelpers::UpdateBuffer(commandBuffer, indexBuffer,
                    GetByteView(indices), SyncScope::kAccelerationStructureBuild);

            BufferHelpers::UpdateBuffer(commandBuffer, materialBuffer,
                    GetByteView(material.factors), SyncScope::kRayTracingShaderRead);
        });

    const GeometryVertexData geometryVertices{
        vertexBuffer,
        RenderObject::KVertexFormat,
        renderObject.GetVertexCount(),
        sizeof(VertexData)
    };

    const GeometryIndexData geometryIndices{
        indexBuffer,
        RenderObject::kIndexType,
        renderObject.GetIndexCount()
    };

    AccelerationStructureManager &asManager = *VulkanContext::accelerationStructureManager;
    const vk::AccelerationStructureKHR blas = asManager.GenerateBlas(geometryVertices, geometryIndices);

    const RenderObjectEntry entry{
        vertexBuffer,
        indexBuffer,
        materialBuffer,
        GeometryInstanceData{
            blas, transform
        }
    };

    renderObjects.emplace(&renderObject, entry);
}

void PathTracingSystem::TraceRays(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    const std::vector<vk::DescriptorSet> descriptorSets{
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

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rayTracingPipeline->Get());

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR,
            rayTracingPipeline->GetLayout(), 0, descriptorSets, {});

    commandBuffer.pushConstants(rayTracingPipeline->GetLayout(),
            vk::ShaderStageFlagBits::eRaygenKHR, 0, vk::ArrayProxy<const uint32_t>{ accumulationIndex++ });

    const ShaderBindingTable &sbt = rayTracingPipeline->GetShaderBindingTable();

    const vk::StridedBufferRegionKHR raygenSBT(sbt.buffer, sbt.raygenOffset, sbt.stride, sbt.stride);
    const vk::StridedBufferRegionKHR missSBT(sbt.buffer, sbt.missOffset, sbt.stride, sbt.stride);
    const vk::StridedBufferRegionKHR hitSBT(sbt.buffer, sbt.hitOffset, sbt.stride, sbt.stride);

    const vk::Extent2D &extent = VulkanContext::swapchain->GetExtent();

    commandBuffer.traceRaysKHR(raygenSBT, missSBT, hitSBT,
            vk::StridedBufferRegionKHR(), extent.height, extent.width, 1);
}

void PathTracingSystem::HandleResizeEvent(const vk::Extent2D &extent)
{
    if (extent.width != 0 && extent.height != 0)
    {
        ResetAccumulation();

        DescriptorHelpers::DestroyMultiDescriptorSet(renderTargets.multiDescriptor);
        DescriptorHelpers::DestroyDescriptorSet(accumulationTarget.descriptor);

        VulkanContext::imageManager->DestroyImage(accumulationTarget.image);

        SetupRenderTargets();
        SetupAccumulationTarget();
    }
}

void PathTracingSystem::ResetAccumulation()
{
    accumulationIndex = 0;
}
