
#include "Engine/Scene/SceneRT.hpp"

#include "Engine/Camera.hpp"
#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Shaders/PathTracing/PathTracing.h"

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
    std::vector<vk::DescriptorSet> descriptors;
    descriptors.reserve(description.descriptorSets.size());

    for (const auto& [type, descriptorSet] : description.descriptorSets)
    {
        descriptors.push_back(descriptorSet.value);
    }

    return descriptors;
}

void SceneRT::UpdateCameraBuffer(vk::CommandBuffer commandBuffer) const
{
    const ShaderDataRT::Camera cameraData{
        glm::inverse(camera->GetViewMatrix()),
        glm::inverse(camera->GetProjectionMatrix()),
        camera->GetDescription().zNear,
        camera->GetDescription().zFar
    };

    BufferHelpers::UpdateBuffer(commandBuffer, description.references.cameraBuffer,
            ByteView(cameraData), SyncScope::kRayTracingShaderRead);
}

SceneRT::SceneRT(Camera* camera_, const Description& description_)
    : camera(camera_)
    , description(description_)
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
