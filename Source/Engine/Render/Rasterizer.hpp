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

    struct GlobalDescriptorSet
    {
        vk::DescriptorSetLayout layout;
        vk::DescriptorSet descriptors;
    };

    struct RenderObjectsDescriptorSets
    {
        vk::DescriptorSetLayout layout;
        std::map<const RenderObject *, vk::DescriptorSet> descriptors;
    };

    Scene &scene;
    Camera &camera;

    RenderPassAttachment depthAttachment;
    std::unique_ptr<RenderPass> renderPass;
    std::vector<vk::Framebuffer> framebuffers;

    vk::Buffer viewProjBuffer;
    vk::Buffer lightingBuffer;

    GlobalDescriptorSet globalDescriptorSet;
    RenderObjectsDescriptorSets renderObjectsDescriptorSets;

    std::unique_ptr<GraphicsPipeline> graphicsPipeline;

    void CreateGlobalDescriptorSet();
    void CreateRenderObjectsDescriptorSets();

    void ExecuteRenderPass(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};
