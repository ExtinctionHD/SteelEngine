#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class Scene;
class AABBox;
class RenderPass;
class GraphicsPipeline;
class DescriptorProvider;

class OcclusionRenderer
{
public:
    OcclusionRenderer(const Scene* scene_);
    ~OcclusionRenderer();

    bool ContainsGeometry(const AABBox& bbox) const;

private:
    const Scene* scene = nullptr;

    Texture depthTexture;
    std::unique_ptr<RenderPass> renderPass;
    vk::Framebuffer framebuffer;

    vk::QueryPool queryPool;

    vk::Buffer cameraBuffer;

    std::unique_ptr<GraphicsPipeline> pipeline;

    std::unique_ptr<DescriptorProvider> descriptorProvider;

    void Render(vk::CommandBuffer commandBuffer) const;
};
