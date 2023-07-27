#include "Engine/Scene/Scene.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Components/EnvironmentComponent.hpp"
#include "Engine/Scene/GlobalIllumination.hpp"
#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/SceneLoader.hpp"
#include "Engine/Scene/SceneMerger.hpp"

Scene::Scene(const Filepath& path)
{
    SceneLoader sceneLoader(*this, path);
}

Scene::~Scene()
{
    for (const auto [entity, ec] : view<EnvironmentComponent>().each())
    {
        VulkanContext::textureManager->DestroyTexture(ec.cubemapTexture);
        VulkanContext::textureManager->DestroyTexture(ec.irradianceTexture);
        VulkanContext::textureManager->DestroyTexture(ec.reflectionTexture);
    }

    for (const auto [entity, lvc] : view<LightVolumeComponent>().each())
    {
        VulkanContext::bufferManager->DestroyBuffer(lvc.coefficientsBuffer);
        VulkanContext::bufferManager->DestroyBuffer(lvc.tetrahedralBuffer);
        VulkanContext::bufferManager->DestroyBuffer(lvc.positionsBuffer);
    }

    const auto& tsc = ctx().get<TextureStorageComponent>();

    for (const Texture& texture : tsc.textures)
    {
        VulkanContext::textureManager->DestroyTexture(texture);
    }

    for (const vk::Sampler sampler : tsc.samplers)
    {
        VulkanContext::textureManager->DestroySampler(sampler);
    }
}

void Scene::AddScene(Scene&& scene, entt::entity spawn)
{
    SceneMerger sceneAdder(std::move(scene), *this, spawn);
}
