#include "Engine/Scene/SceneRT.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

SceneRT::SceneRT(const Description& description_)
    : description(description_)
{}

SceneRT::~SceneRT()
{
    DescriptorHelpers::DestroyDescriptorSet(description.descriptorSet);

    for (const auto& accelerationStructure : description.resources.accelerationStructures)
    {
        VulkanContext::accelerationStructureManager->DestroyAccelerationStructure(accelerationStructure);
    }

    for (const auto& buffer : description.resources.buffers)
    {
        VulkanContext::bufferManager->DestroyBuffer(buffer);
    }

    for (const auto& sampler : description.resources.samplers)
    {
        VulkanContext::textureManager->DestroySampler(sampler);
    }

    for (const auto& texture : description.resources.textures)
    {
        VulkanContext::textureManager->DestroyTexture(texture);
    }
}
