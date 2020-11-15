#include "Engine/Scene/Scene.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Camera.hpp"

const std::vector<vk::Format> Scene::Mesh::Vertex::kFormat{
    vk::Format::eR32G32B32Sfloat,
    vk::Format::eR32G32B32Sfloat,
    vk::Format::eR32G32B32Sfloat,
    vk::Format::eR32G32Sfloat,
};

void Scene::UpdateCameraBuffer(vk::CommandBuffer commandBuffer) const
{
    const glm::mat4 viewProj = camera->GetProjectionMatrix() * camera->GetViewMatrix();

    BufferHelpers::UpdateBuffer(commandBuffer, description.references.cameraBuffer,
            ByteView(viewProj), SyncScope::kVertexShaderRead);
}

Scene::Scene(Camera* camera_, const Description& description_)
    : camera(camera_)
    , description(description_)
{}

Scene::~Scene()
{
    DescriptorHelpers::DestroyDescriptorSet(description.descriptorSets.camera);
    DescriptorHelpers::DestroyDescriptorSet(description.descriptorSets.environment);
    DescriptorHelpers::DestroyMultiDescriptorSet(description.descriptorSets.materials);

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
