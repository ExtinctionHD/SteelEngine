#pragma once

class Window;
class RenderPass;
class Scene;

class ImGuiRenderer
{
public:
    using TextBinding = std::function<std::string()>;

    ImGuiRenderer(const Window& window);
    ~ImGuiRenderer();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

private:
    vk::DescriptorPool descriptorPool;
    std::unique_ptr<RenderPass> renderPass;
    std::vector<vk::Framebuffer> framebuffers;

    void BuildFrame() const;

    void HandleResizeEvent(const vk::Extent2D& extent);
};
