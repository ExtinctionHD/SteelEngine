#include "Engine/Scene/SceneRT.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

std::vector<vk::DescriptorSetLayout> SceneRT::GetDescriptorSetLayouts() const
{
    std::vector<vk::DescriptorSetLayout> layouts;
    layouts.reserve(description.descriptorSets.size());

    for (const auto& [type, descriptorSet] : description.descriptorSets)
    {
        layouts.push_back(descriptorSet.layout);
    }

    return layouts;
}

std::vector<vk::DescriptorSet> SceneRT::GetDescriptorSets() const
{
    std::vector<vk::DescriptorSet> descriptorSets;
    descriptorSets.reserve(description.descriptorSets.size());

    for (const auto& [type, descriptorSet] : description.descriptorSets)
    {
        descriptorSets.push_back(descriptorSet.value);
    }

    return descriptorSets;
}

SceneRT::SceneRT(const Description& description_)
    : description(description_)
{}

SceneRT::~SceneRT()
{
    for (const auto& [type, descriptorSet] : description.descriptorSets)
    {
        DescriptorHelpers::DestroyDescriptorSet(descriptorSet);
    }

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
