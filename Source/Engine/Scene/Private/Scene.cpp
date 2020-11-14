#include "Engine/Scene/Scene.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Camera.hpp"

Scene::Scene(Camera *camera_, const Description& description_)
    : camera(camera_)
    , description(description_)
{}

Scene::~Scene()
{
    for (const auto& mesh : description.meshes)
    {
        VulkanContext::bufferManager->DestroyBuffer(mesh.indexBuffer);
        VulkanContext::bufferManager->DestroyBuffer(mesh.vertexBuffer);
    }

    for (const auto& material : description.materials)
    {
        VulkanContext::bufferManager->DestroyBuffer(material.factorsBuffer);
    }

    for (const auto& renderObject : description.renderObjects)
    {
        VulkanContext::bufferManager->DestroyBuffer(renderObject.transformBuffer);
    }

    for (const auto& texture : description.textures)
    {
        VulkanContext::textureManager->DestroyTexture(texture);
    }

    for (const auto& sampler : description.samplers)
    {
        VulkanContext::textureManager->DestroySampler(sampler);
    }
}
