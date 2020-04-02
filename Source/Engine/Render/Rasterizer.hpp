#pragma once

#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/RenderObject.hpp"

class Scene;
class Camera;
class RenderPass;
class GraphicsPipeline;

class Rasterizer
        : public Renderer
{
public:
    Rasterizer(Scene &scene_, Camera &camera_);
    ~Rasterizer();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) override;

    void OnResize(const vk::Extent2D &extent) override;

private:
    struct RenderPassAttachment
    {
        vk::Image image;
        vk::ImageView view;
    };

    struct GlobalUniforms
    {
        vk::DescriptorSet descriptorSet;
        vk::Buffer viewProjBuffer;
        vk::Buffer lightingBuffer;
    };

    struct RenderObjectUniforms
    {
        vk::DescriptorSet descriptorSet;
        vk::Buffer transformBuffer;
        vk::Buffer materialBuffer;
    };

    Scene &scene;
    Camera &camera;

    RenderPassAttachment depthAttachment;
    std::unique_ptr<RenderPass> renderPass;
    std::vector<vk::Framebuffer> framebuffers;

    vk::DescriptorSetLayout globalLayout;
    GlobalUniforms globalUniforms;

    vk::DescriptorSetLayout renderObjectLayout;
    std::map<const RenderObject *, RenderObjectUniforms> renderObjects;

    std::unique_ptr<GraphicsPipeline> graphicsPipeline;

    void SetupGlobalUniforms();
    void SetupRenderObjects();

    void ExecuteRenderPass(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};
