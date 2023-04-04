#include "Engine/Render/RenderHelpers.hpp"

#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Scene/GlobalIllumination.hpp"
#include "Engine/Scene/ImageBasedLighting.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/StorageComponents.hpp"

CameraData RenderHelpers::CreateCameraData(uint32_t bufferCount,
        vk::DeviceSize bufferSize, vk::ShaderStageFlags shaderStages)
{
    std::vector<vk::Buffer> buffers(bufferCount);

    const DescriptorDescription descriptorDescription{
        1, vk::DescriptorType::eUniformBuffer,
        shaderStages,
        vk::DescriptorBindingFlags()
    };

    std::vector<DescriptorSetData> multiDescriptorSetData(bufferCount);

    for (uint32_t i = 0; i < bufferCount; ++i)
    {
        buffers[i] = BufferHelpers::CreateEmptyBuffer(
                vk::BufferUsageFlagBits::eUniformBuffer, bufferSize);

        const DescriptorData descriptorData = DescriptorHelpers::GetData(buffers[i]);

        multiDescriptorSetData[i] = DescriptorSetData{ descriptorData };
    }

    const MultiDescriptorSet descriptorSet = DescriptorHelpers::CreateMultiDescriptorSet(
            { descriptorDescription }, multiDescriptorSetData);

    return CameraData{ buffers, descriptorSet };
}

vk::Rect2D RenderHelpers::GetSwapchainRenderArea()
{
    return vk::Rect2D(vk::Offset2D(), VulkanContext::swapchain->GetExtent());
}

vk::Viewport RenderHelpers::GetSwapchainViewport()
{
    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    return vk::Viewport(0.0f, 0.0f,
            static_cast<float>(extent.width),
            static_cast<float>(extent.height),
            0.0f, 1.0f);
}

DescriptorSet RenderHelpers::CreateMaterialDescriptorSet(
        const Scene& scene, vk::ShaderStageFlags stageFlags)
{
    const auto& textureComponent = scene.ctx().get<TextureStorageComponent>();
    const auto& renderComponent = scene.ctx().get<RenderStorageComponent>();

    const uint32_t textureCount = static_cast<uint32_t>(textureComponent.textures.size());

    const DescriptorSetDescription descriptorSetDescription{
        DescriptorDescription{
            textureCount,
            vk::DescriptorType::eCombinedImageSampler,
            stageFlags,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            stageFlags,
            vk::DescriptorBindingFlags()
        }
    };

    const DescriptorSetData descriptorSetData{
        DescriptorHelpers::GetData(textureComponent.textures),
        DescriptorHelpers::GetData(renderComponent.materialBuffer)
    };

    return DescriptorHelpers::CreateDescriptorSet(descriptorSetDescription, descriptorSetData);
}

DescriptorSet RenderHelpers::CreateLightingDescriptorSet(
        const Scene& scene, vk::ShaderStageFlags stageFlags)
{
    const auto& environmentComponent = scene.ctx().get<EnvironmentComponent>();
    const auto& renderComponent = scene.ctx().get<RenderStorageComponent>();

    const ImageBasedLighting& imageBasedLighting = *RenderContext::imageBasedLighting;

    const ImageBasedLighting::Samplers& iblSamplers = imageBasedLighting.GetSamplers();

    const Texture& irradianceTexture = environmentComponent.irradianceTexture;
    const Texture& reflectionTexture = environmentComponent.reflectionTexture;
    const Texture& specularBRDF = imageBasedLighting.GetSpecularBRDF();

    DescriptorSetDescription descriptorSetDescription{
        DescriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            stageFlags,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            stageFlags,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            stageFlags,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            stageFlags,
            vk::DescriptorBindingFlags()
        },
    };

    DescriptorSetData descriptorSetData{
        DescriptorHelpers::GetData(renderComponent.lightBuffer),
        DescriptorHelpers::GetData(iblSamplers.irradiance, irradianceTexture.view),
        DescriptorHelpers::GetData(iblSamplers.reflection, reflectionTexture.view),
        DescriptorHelpers::GetData(iblSamplers.specularBRDF, specularBRDF.view),
    };

    if (scene.ctx().contains<LightVolumeComponent>())
    {
        const auto& lightVolumeComponent = scene.ctx().get<LightVolumeComponent>();

        descriptorSetDescription.push_back(DescriptorDescription{
            1, vk::DescriptorType::eStorageBuffer,
            stageFlags,
            vk::DescriptorBindingFlags()
        });
        descriptorSetDescription.push_back(DescriptorDescription{
            1, vk::DescriptorType::eStorageBuffer,
            stageFlags,
            vk::DescriptorBindingFlags()
        });
        descriptorSetDescription.push_back(DescriptorDescription{
            1, vk::DescriptorType::eStorageBuffer,
            stageFlags,
            vk::DescriptorBindingFlags()
        });

        descriptorSetData.push_back(DescriptorHelpers::GetStorageData(lightVolumeComponent.positionsBuffer));
        descriptorSetData.push_back(DescriptorHelpers::GetStorageData(lightVolumeComponent.tetrahedralBuffer));
        descriptorSetData.push_back(DescriptorHelpers::GetStorageData(lightVolumeComponent.coefficientsBuffer));
    }

    return DescriptorHelpers::CreateDescriptorSet(descriptorSetDescription, descriptorSetData);
}

DescriptorSet RenderHelpers::CreateRayTracingDescriptorSet(
        const Scene& scene, vk::ShaderStageFlags stageFlags,
        bool includeMaterialBuffer)
{
    const auto& geometryComponent = scene.ctx().get<GeometryStorageComponent>();
    const auto& textureComponent = scene.ctx().get<TextureStorageComponent>();
    const auto& renderComponent = scene.ctx().get<RenderStorageComponent>();

    const uint32_t textureCount = static_cast<uint32_t>(textureComponent.textures.size());
    const uint32_t primitiveCount = static_cast<uint32_t>(geometryComponent.primitives.size());

    DescriptorSetDescription descriptorSetDescription{
        DescriptorDescription{
            1, vk::DescriptorType::eAccelerationStructureKHR,
            stageFlags,
            vk::DescriptorBindingFlags()
        }
    };

    if (includeMaterialBuffer)
    {
        descriptorSetDescription.push_back(DescriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            stageFlags,
            vk::DescriptorBindingFlags()
        });
    }
    
    descriptorSetDescription.push_back(DescriptorDescription{
        textureCount, vk::DescriptorType::eCombinedImageSampler,
        stageFlags,
        vk::DescriptorBindingFlags()
    });
    descriptorSetDescription.push_back(DescriptorDescription{
        primitiveCount, vk::DescriptorType::eStorageBuffer,
        stageFlags,
        vk::DescriptorBindingFlags()
    });
    descriptorSetDescription.push_back(DescriptorDescription{
        primitiveCount, vk::DescriptorType::eStorageBuffer,
        stageFlags,
        vk::DescriptorBindingFlags()
    });

    std::vector<vk::Buffer> indexBuffers;
    std::vector<vk::Buffer> texCoordBuffers;

    indexBuffers.reserve(geometryComponent.primitives.size());
    texCoordBuffers.reserve(geometryComponent.primitives.size());

    for (const auto& primitive : geometryComponent.primitives)
    {
        indexBuffers.push_back(primitive.indexBuffer);
        texCoordBuffers.push_back(primitive.texCoordBuffer);
    }

    DescriptorSetData descriptorSetData{
        DescriptorHelpers::GetData(renderComponent.tlas)
    };

    if (includeMaterialBuffer)
    {
        descriptorSetData.push_back(DescriptorHelpers::GetData(renderComponent.materialBuffer));
    }
    
    descriptorSetData.push_back(DescriptorHelpers::GetData(textureComponent.textures));
    descriptorSetData.push_back(DescriptorHelpers::GetStorageData(indexBuffers));
    descriptorSetData.push_back(DescriptorHelpers::GetStorageData(texCoordBuffers));

    return DescriptorHelpers::CreateDescriptorSet(descriptorSetDescription, descriptorSetData);
}

std::vector<MaterialPipeline> RenderHelpers::CreateMaterialPipelines(
        const Scene& scene, const RenderPass& renderPass,
        const std::vector<vk::DescriptorSetLayout>& layouts,
        const CreateMaterialPipelinePred& createPipelinePred,
        const MaterialPipelineCreator& pipelineCreator)
{
    std::vector<MaterialPipeline> pipelines;

    const auto& materialComponent = scene.ctx().get<MaterialStorageComponent>();

    for (const auto& material : materialComponent.materials)
    {
        if (createPipelinePred(material.flags))
        {
            const auto pred = [&material](const MaterialPipeline& materialPipeline)
                {
                    return materialPipeline.materialFlags == material.flags;
                };

            const auto it = std::ranges::find_if(pipelines, pred);

            if (it == pipelines.end())
            {
                pipelines.emplace_back(material.flags, pipelineCreator(
                        renderPass, layouts, material.flags, scene));
            }
        }
    }

    return pipelines;
}
