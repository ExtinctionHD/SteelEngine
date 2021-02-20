#include "Engine/Scene/Scene.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

const std::vector<vk::Format> Scene::Mesh::Vertex::kFormat{
    vk::Format::eR32G32B32Sfloat,
    vk::Format::eR32G32B32Sfloat,
    vk::Format::eR32G32B32Sfloat,
    vk::Format::eR32G32Sfloat,
};

bool Scene::PipelineState::operator==(const PipelineState& other) const
{
    return alphaTest == other.alphaTest && doubleSided == other.doubleSided;
}

Scene::Scene(const Description& description_)
    : description(description_)
{}

Scene::~Scene()
{
    DescriptorHelpers::DestroyDescriptorSet(description.descriptorSets.rayTracing);
    DescriptorHelpers::DestroyMultiDescriptorSet(description.descriptorSets.materials);
    if (description.descriptorSets.pointLights.has_value())
    {
        DescriptorHelpers::DestroyDescriptorSet(description.descriptorSets.pointLights.value());
    }

    for (const auto& accelerationStructure : description.resources.accelerationStructures)
    {
        VulkanContext::accelerationStructureManager->DestroyAccelerationStructure(accelerationStructure);
    }

    for (const auto& buffer : description.resources.buffers)
    {
        VulkanContext::bufferManager->DestroyBuffer(buffer);
    }

    for (const auto& texture : description.resources.textures)
    {
        VulkanContext::textureManager->DestroyTexture(texture);
    }

    for (const auto& sampler : description.resources.samplers)
    {
        VulkanContext::textureManager->DestroySampler(sampler);
    }
}

std::vector<Scene::RenderObject> Scene::GetRenderObjects(uint32_t materialIndex) const
{
    const std::vector<RenderObject>& renderObjects = description.hierarchy.renderObjects;

    std::vector<RenderObject> result;

    const auto pred = [materialIndex](const Scene::RenderObject& renderObject)
        {
            return renderObject.materialIndex == materialIndex;
        };

    std::copy_if(renderObjects.begin(), renderObjects.end(), std::back_inserter(result), pred);

    return result;
}
