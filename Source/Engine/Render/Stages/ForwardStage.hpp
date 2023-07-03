#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"

class Scene;
class RenderPass;
class GraphicsPipeline;

class ForwardStage
{
public:
    ForwardStage(vk::ImageView depthImageView);

    ~ForwardStage();

    void RegisterScene(const Scene* scene_);

    void RemoveScene();

    void Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void Resize(vk::ImageView depthImageView);

    void ReloadShaders();

private:
    struct EnvironmentData
    {
        vk::Buffer indexBuffer;
    };

    static EnvironmentData CreateEnvironmentData();

    const Scene* scene = nullptr;

    std::unique_ptr<RenderPass> renderPass;
    std::vector<vk::Framebuffer> framebuffers;

    EnvironmentData environmentData;

    std::vector<MaterialPipeline> materialPipelines;
    std::unique_ptr<GraphicsPipeline> environmentPipeline;

    std::unique_ptr<DescriptorProvider> materialDescriptorProvider;
    std::unique_ptr<DescriptorProvider> environmentDescriptorProvider;

    void DrawScene(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
    void DrawEnvironment(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};
