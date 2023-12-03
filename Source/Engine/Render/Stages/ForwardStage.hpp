#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/Vulkan/Pipelines/MaterialPipelineCache.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

class Scene;
class RenderPass;
class GraphicsPipeline;

class ForwardStage
{
public:
    ForwardStage(const RenderTarget& depthTarget);

    ~ForwardStage();

    void RegisterScene(const Scene* scene_);

    void RemoveScene();

    void Update();

    void Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void Resize(const RenderTarget& depthTarget);

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

    std::unique_ptr<MaterialPipelineCache> materialPipelineCache;
    std::set<MaterialFlags> uniqueMaterialPipelines;

    EnvironmentData environmentData;
    std::unique_ptr<GraphicsPipeline> environmentPipeline;
    std::unique_ptr<DescriptorProvider> environmentDescriptorProvider;

    void DrawScene(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
    void DrawEnvironment(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};
