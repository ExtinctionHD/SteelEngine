#pragma once

#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class AABBox;
class Camera;
class Scene;
class RenderPass;
class GraphicsPipeline;

class OcclusionRenderer
{
public:
    OcclusionRenderer(const Scene* scene_);
    ~OcclusionRenderer();

    bool ContainsGeometry(const AABBox& bbox) const;

private:
    struct CameraData
    {
        vk::Buffer buffer;
        DescriptorSet descriptorSet;
    };

    const Scene* scene = nullptr;

    Texture depthTexture;
    std::unique_ptr<RenderPass> renderPass;
    vk::Framebuffer framebuffer;

    vk::QueryPool queryPool;

    std::unique_ptr<Camera> camera;
    CameraData cameraData;

    std::unique_ptr<GraphicsPipeline> pipeline;

    void PlaceCamera(const AABBox& bbox, int32_t directionAxis) const;

    void Render(vk::CommandBuffer commandBuffer) const;
};
