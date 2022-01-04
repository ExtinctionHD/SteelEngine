#include "Engine/Scene/GlobalIllumination.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/ProbeRenderer.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Scene/MeshHelpers.hpp"
#include "Engine/Scene/ScenePT.hpp"

#include "Utils/AABBox.hpp"
#include "Utils/TimeHelpers.hpp"

namespace Details
{
    static constexpr float kBBoxExtension = 0.2f;

    static constexpr float kMaxVolumeStep = 2.0f;

    static constexpr uint32_t kCoefficientCount = COEFFICIENT_COUNT;
    static constexpr uint32_t kTextureCount = kCoefficientCount;

    static const Filepath kIrradianceVolumeShaderPath("~/Shaders/Compute/ImageBasedLighting/IrradianceVolume.comp");
    static const Filepath kLightVolumeShaderPath("~/Shaders/Compute/ImageBasedLighting/LightVolume.comp");

    static vk::Sampler CreateIrradianceVolumeSampler()
    {
        const SamplerDescription description{
            vk::Filter::eLinear,
            vk::Filter::eLinear,
            vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eClampToEdge,
            std::nullopt, 0.0f, 0.0f, false
        };

        return VulkanContext::textureManager->CreateSampler(description);
    }

    static vk::DescriptorSetLayout CreateProbeLayout()
    {
        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        return VulkanContext::descriptorPool->CreateDescriptorSetLayout({ descriptorDescription });
    }

    static vk::DescriptorSetLayout CreateTexturesLayout()
    {
        const DescriptorDescription descriptorDescription{
            kTextureCount, vk::DescriptorType::eStorageImage,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlagBits::eVariableDescriptorCount
        };

        return VulkanContext::descriptorPool->CreateDescriptorSetLayout({ descriptorDescription });
    }

    static vk::DescriptorSetLayout CreateCoefficientsLayout()
    {
        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eStorageBuffer,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        return VulkanContext::descriptorPool->CreateDescriptorSetLayout({ descriptorDescription });
    }

    static std::unique_ptr<ComputePipeline> CreateIrradianceVolumePipeline(
            const std::vector<vk::DescriptorSetLayout>& layouts)
    {
        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute, kIrradianceVolumeShaderPath, {});

        const vk::PushConstantRange pushConstantRange{
            vk::ShaderStageFlagBits::eCompute, 0, sizeof(glm::uvec3)
        };

        const ComputePipeline::Description pipelineDescription{
            shaderModule, layouts, { pushConstantRange }
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(pipelineDescription);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static std::unique_ptr<ComputePipeline> CreateLightVolumePipeline(
            const std::vector<vk::DescriptorSetLayout>& layouts)
    {
        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute, kLightVolumeShaderPath, {});

        const vk::PushConstantRange pushConstantRange{
            vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)
        };

        const ComputePipeline::Description pipelineDescription{
            shaderModule, layouts, { pushConstantRange }
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(pipelineDescription);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static AABBox GetVolumeBBox(const AABBox& sceneBBox)
    {
        AABBox bbox = sceneBBox;
        bbox.Extend(kBBoxExtension);
        return bbox;
    }

    static glm::uvec3 GetVolumeSize(const AABBox& bbox)
    {
        return glm::uvec3(glm::ceil(bbox.GetSize() / kMaxVolumeStep)) + glm::uvec3(1);
    }

    static glm::vec3 GetVolumeStep(const AABBox& bbox, const glm::uvec3& size)
    {
        return bbox.GetSize() / glm::vec3(size - glm::uvec3(1));
    }

    static AABBox FixVolumeBBox(const AABBox& volumeBBox)
    {
        const glm::uvec3 size = GetVolumeSize(volumeBBox);
        const glm::vec3 step = GetVolumeStep(volumeBBox, size);

        AABBox bbox = volumeBBox;
        bbox.Extend(step * 0.5f);
        return bbox;
    }

    static std::vector<IrradianceVolume::Vertex> GenerateIrradianceVolumeVertices(const AABBox& bbox)
    {
        const glm::uvec3 size = GetVolumeSize(bbox);
        const glm::vec3 step = GetVolumeStep(bbox, size);

        std::vector<IrradianceVolume::Vertex> vertices;
        vertices.reserve(size.x * size.y * size.z);

        for (uint32_t i = 0; i < size.x; ++i)
        {
            for (uint32_t j = 0; j < size.y; ++j)
            {
                for (uint32_t k = 0; k < size.z; ++k)
                {
                    const IrradianceVolume::Vertex vertex{
                        bbox.GetMin() + glm::vec3(i, j, k) * step,
                        glm::uvec3(i, j, k)
                    };

                    vertices.push_back(vertex);
                }
            }
        }

        return vertices;
    }

    static std::vector<glm::vec3> GenerateLightVolumePositions(const AABBox& bbox)
    {
        const glm::uvec3 size = GetVolumeSize(bbox);
        const glm::vec3 step = GetVolumeStep(bbox, size);

        std::vector<glm::vec3> positions;
        positions.reserve(size.x * size.y * size.z);

        for (uint32_t i = 0; i < size.x; ++i)
        {
            for (uint32_t j = 0; j < size.y; ++j)
            {
                for (uint32_t k = 0; k < size.z; ++k)
                {
                    const glm::vec3 position = bbox.GetMin() + glm::vec3(i, j, k) * step;

                    positions.push_back(position);
                }
            }
        }

        return positions;
    }

    static std::vector<Texture> CreateIrradianceVolumeTextures(const AABBox& bbox)
    {
        const glm::uvec3 size = GetVolumeSize(bbox);

        const vk::Extent3D extent(size.x, size.y, size.z);

        constexpr vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

        const ImageDescription description{
            ImageType::e3D, vk::Format::eR16G16B16A16Sfloat,
            extent, 1, 1, vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal, usage,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        std::vector<Texture> textures(kTextureCount);

        for (auto& texture : textures)
        {
            texture.image = VulkanContext::imageManager->CreateImage(
                    description, ImageCreateFlags::kNone);

            texture.view = VulkanContext::imageManager->CreateView(
                    texture.image, vk::ImageViewType::e3D, ImageHelpers::kFlatColor);
        }

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eGeneral,
                    PipelineBarrier::kEmpty
                };

                for (const auto& texture : textures)
                {
                    ImageHelpers::TransitImageLayout(commandBuffer,
                            texture.image, ImageHelpers::kFlatColor, layoutTransition);
                }
            });

        return textures;
    }

    static vk::Buffer CreateLightVolumeCoefficientsBuffer(uint32_t probeCount)
    {
        const uint32_t size = probeCount * kCoefficientCount * sizeof(glm::vec3);

        const BufferDescription description{
            size, vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        return VulkanContext::bufferManager->CreateBuffer(description, BufferCreateFlags::kNone);
    }

    static vk::DescriptorSet AllocateTexturesDescriptorSet(
            vk::DescriptorSetLayout layout, const std::vector<Texture>& textures)
    {
        const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const vk::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSets({ layout }).front();

        const std::vector<vk::ImageView> views = TextureHelpers::GetViews(textures);

        const DescriptorData descriptorData = DescriptorHelpers::GetStorageData(views);

        descriptorPool.UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return descriptorSet;
    }

    static vk::DescriptorSet AllocateCoefficientsDescriptorSet(
            vk::DescriptorSetLayout layout, vk::Buffer buffer)
    {
        const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const vk::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSets({ layout }).front();

        const DescriptorData descriptorData = DescriptorHelpers::GetStorageData(buffer);

        descriptorPool.UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return descriptorSet;
    }

    static vk::DescriptorSet AllocateProbeDescriptorSet(
            vk::DescriptorSetLayout layout, vk::ImageView probeView)
    {
        const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const vk::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSets({ layout }).front();

        const DescriptorData descriptorData =
                DescriptorHelpers::GetData(RenderContext::defaultSampler, probeView);

        descriptorPool.UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return descriptorSet;
    }
}

GlobalIllumination::GlobalIllumination()
{
    irradianceVolumeSampler = Details::CreateIrradianceVolumeSampler();

    probeLayout = Details::CreateProbeLayout();
    texturesLayout = Details::CreateTexturesLayout();
    coefficientsLayout = Details::CreateCoefficientsLayout();

    irradianceVolumePipeline = Details::CreateIrradianceVolumePipeline({ probeLayout, texturesLayout });
    lightVolumePipeline = Details::CreateLightVolumePipeline({ probeLayout, coefficientsLayout });
}

GlobalIllumination::~GlobalIllumination()
{
    VulkanContext::textureManager->DestroySampler(irradianceVolumeSampler);

    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(probeLayout);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(texturesLayout);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(coefficientsLayout);
}

IrradianceVolume GlobalIllumination::GenerateIrradianceVolume(
        ScenePT* scene, Environment* environment) const
{
    ScopeTime scopeTime("GlobalIllumination::GenerateIrradianceVolume");

    const std::unique_ptr<ProbeRenderer> probeRenderer = std::make_unique<ProbeRenderer>(scene, environment);

    const AABBox bbox = Details::GetVolumeBBox(scene->GetInfo().bbox);
    const std::vector<IrradianceVolume::Vertex> vertices = Details::GenerateIrradianceVolumeVertices(bbox);
    const std::vector<Texture> textures = Details::CreateIrradianceVolumeTextures(bbox);

    const vk::DescriptorSet texturesDescriptorSet
            = Details::AllocateTexturesDescriptorSet(texturesLayout, textures);

    ProgressLogger progressLogger("GlobalIllumination::GenerateIrradianceVolume", 1.0f);

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        const IrradianceVolume::Vertex& vertex = vertices[i];

        const Texture probeTexture = probeRenderer->CaptureProbe(vertex.position);

        const vk::DescriptorSet probeDescriptorSet
                = Details::AllocateProbeDescriptorSet(probeLayout, probeTexture.view);

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, irradianceVolumePipeline->Get());

                commandBuffer.pushConstants<glm::uvec3>(irradianceVolumePipeline->GetLayout(),
                        vk::ShaderStageFlagBits::eCompute, 0, { vertex.coord });

                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                        irradianceVolumePipeline->GetLayout(), 0, { probeDescriptorSet, texturesDescriptorSet }, {});

                commandBuffer.dispatch(1, 1, 1);
            });

        VulkanContext::descriptorPool->FreeDescriptorSets({ probeDescriptorSet });
        VulkanContext::textureManager->DestroyTexture(probeTexture);

        progressLogger.Log(i, vertices.size());
    }

    progressLogger.End();

    VulkanContext::descriptorPool->FreeDescriptorSets({ texturesDescriptorSet });

    for (size_t i = 0; i < textures.size(); ++i)
    {
        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eGeneral,
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    PipelineBarrier::kEmpty
                };

                ImageHelpers::TransitImageLayout(commandBuffer,
                        textures[i].image, ImageHelpers::kFlatColor, layoutTransition);
            });

        const std::string imageName = "IrradianceVolume_" + std::to_string(i);

        VulkanHelpers::SetObjectName(VulkanContext::device->Get(), textures[i].image, imageName);
    }

    return IrradianceVolume{ Details::FixVolumeBBox(bbox), vertices, textures };
}

LightVolume GlobalIllumination::GenerateLightVolume(
        ScenePT* scene, Environment* environment) const
{
    ScopeTime scopeTime("GlobalIllumination::GenerateLightVolume");

    const std::unique_ptr<ProbeRenderer> probeRenderer = std::make_unique<ProbeRenderer>(scene, environment);

    const AABBox bbox = Details::GetVolumeBBox(scene->GetInfo().bbox);
    const std::vector<glm::vec3> positions = Details::GenerateLightVolumePositions(bbox);
    const std::vector<Tetrahedron> tetrahedral = MeshHelpers::GenerateTetrahedral(positions);

    const vk::Buffer positionsBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eStorageBuffer, ByteView(positions));
    const vk::Buffer tetrahedralBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eStorageBuffer, ByteView(tetrahedral));

    const uint32_t probeCount = static_cast<uint32_t>(positions.size());
    const vk::Buffer coefficientsBuffer = Details::CreateLightVolumeCoefficientsBuffer(probeCount);

    const vk::DescriptorSet coefficientsDescriptorSet
            = Details::AllocateCoefficientsDescriptorSet(coefficientsLayout, coefficientsBuffer);

    ProgressLogger progressLogger("GlobalIllumination::GenerateLightVolume", 1.0f);

    for (size_t i = 0; i < positions.size(); ++i)
    {
        const Texture probeTexture = probeRenderer->CaptureProbe(positions[i]);

        const vk::DescriptorSet probeDescriptorSet
                = Details::AllocateProbeDescriptorSet(probeLayout, probeTexture.view);

        const std::vector<vk::DescriptorSet> descriptorSets{
            probeDescriptorSet, coefficientsDescriptorSet
        };

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, lightVolumePipeline->Get());

                commandBuffer.pushConstants<uint32_t>(irradianceVolumePipeline->GetLayout(),
                        vk::ShaderStageFlagBits::eCompute, 0, { static_cast<uint32_t>(i) });

                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                        lightVolumePipeline->GetLayout(), 0, descriptorSets, {});

                commandBuffer.dispatch(1, 1, 1);
            });

        VulkanContext::descriptorPool->FreeDescriptorSets({ probeDescriptorSet });
        VulkanContext::textureManager->DestroyTexture(probeTexture);

        progressLogger.Log(i, positions.size());
    }

    progressLogger.End();

    return LightVolume{ positionsBuffer, tetrahedralBuffer, coefficientsBuffer, positions };
}
