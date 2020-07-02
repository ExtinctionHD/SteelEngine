#include "Engine/Scene/SceneRT.hpp"

#include "Engine/Camera.hpp"
#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

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

    for (const auto& [type, descriptorSet] : descriptorSets)
    {
        descriptors.push_back(descriptorSet.value);
    }

    return descriptors;
}

void SceneRT::UpdateCameraBuffer(vk::CommandBuffer commandBuffer) const
{
    const vk::Buffer& cameraBuffer = buffers.front();

    const ShaderData::Camera cameraData{
        glm::inverse(camera->GetViewMatrix()),
        glm::inverse(camera->GetProjectionMatrix()),
        camera->GetDescription().zNear,
        camera->GetDescription().zFar
    };

    BufferHelpers::UpdateBuffer(commandBuffer, cameraBuffer,
            ByteView(cameraData), SyncScope::kRayTracingShaderRead);
}
