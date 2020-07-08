#include "Engine/Scene/SceneRT.hpp"

#include "Engine/Camera.hpp"
#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Shaders/PathTracing/PathTracing.h"

std::vector<vk::DescriptorSetLayout> SceneRT::GetDescriptorSetLayouts() const
{
    std::vector<vk::DescriptorSetLayout> layouts;
    layouts.reserve(descriptorSets.size());

    for (const auto &[type, descriptorSet] : descriptorSets)
    {
        layouts.push_back(descriptorSet.layout);
    }

    return layouts;
}

std::vector<vk::DescriptorSet> SceneRT::GetDescriptorSets() const
{
    std::vector<vk::DescriptorSet> descriptors;
    descriptors.reserve(descriptorSets.size());

    for (const auto &[type, descriptorSet] : descriptorSets)
    {
        descriptors.push_back(descriptorSet.value);
    }

    return descriptors;
}

void SceneRT::UpdateCameraBuffer(vk::CommandBuffer commandBuffer) const
{
    const ShaderData::Camera cameraData{
        glm::inverse(camera->GetViewMatrix()),
        glm::inverse(camera->GetProjectionMatrix()),
        camera->GetDescription().zNear,
        camera->GetDescription().zFar
    };

    BufferHelpers::UpdateBuffer(commandBuffer, references.cameraBuffer,
            ByteView(cameraData), SyncScope::kRayTracingShaderRead);
}

SceneRT::SceneRT(Camera *camera_, const Resources &resources_,
        const References& references_, const DescriptorSets &descriptorSets_)
    : camera(camera_)
    , resources(resources_)
    , references(references_)
    , descriptorSets(descriptorSets_)
{}

SceneRT::~SceneRT()
{
    for (const auto &[type, descriptorSet] : descriptorSets)
    {
        DescriptorHelpers::DestroyDescriptorSet(descriptorSet);
    }

    for (const auto &accelerationStructure : resources.accelerationStructures)
    {
        VulkanContext::accelerationStructureManager->DestroyAccelerationStructure(accelerationStructure);
    }

    for (const auto &buffer : resources.buffers)
    {
        VulkanContext::bufferManager->DestroyBuffer(buffer);
    }

    for (const auto &sampler : resources.samplers)
    {
        VulkanContext::textureManager->DestroySampler(sampler);
    }

    for (const auto &texture : resources.textures)
    {
        VulkanContext::textureManager->DestroyTexture(texture);
    }
}
