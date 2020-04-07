#pragma once

#include "Engine/Render/Renderer.hpp"

class Scene;
class Camera;
class RenderObject;
class RenderPass;
class GraphicsPipeline;

class ForwardRenderPass
        : public Renderer
{
public:
    ForwardRenderPass(Scene &scene_, Camera &camera_);
    ~ForwardRenderPass();

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
    };

    struct RenderObjectEntry
    {
        vk::Buffer vertexBuffer;
        vk::Buffer indexBuffer;
        RenderObjectUniforms uniforms;
    };

    Scene &scene;
    Camera &camera;

    RenderPassAttachment depthAttachment;
    std::unique_ptr<RenderPass> renderPass;
    std::vector<vk::Framebuffer> framebuffers;

    vk::DescriptorSetLayout globalLayout;
    GlobalUniforms globalUniforms;

    vk::DescriptorSetLayout renderObjectLayout;
    std::map<const RenderObject *, RenderObjectEntry> renderObjects;

    std::unique_ptr<GraphicsPipeline> graphicsPipeline;

    void SetupGlobalData();
    void SetupRenderObjects();

    void SetupRenderObject(const RenderObject &renderObject, const glm::mat4 &transform);

    void ExecuteRenderPass(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};
