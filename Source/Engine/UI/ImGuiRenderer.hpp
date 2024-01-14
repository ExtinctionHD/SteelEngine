#pragma once

class Scene;
class Window;
class RenderPass;
class ImGuiWidget;

class ImGuiRenderer
{
public:
    ImGuiRenderer(const Window& window);
    ~ImGuiRenderer();

    void Build(Scene* scene, float deltaSeconds) const;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

private:
    vk::DescriptorPool descriptorPool;
    std::unique_ptr<RenderPass> renderPass;
    std::vector<vk::Framebuffer> framebuffers;

    std::vector<std::unique_ptr<ImGuiWidget>> widgets;

    void HandleResizeEvent(const vk::Extent2D& extent);
};
