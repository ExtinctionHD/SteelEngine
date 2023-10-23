#pragma once

class Window;
class RenderPass;
class Scene;

class UIRenderer
{
public:
    using TextBinding = std::function<std::string()>;

    UIRenderer(const Window& window);
    ~UIRenderer();

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex, Scene* scene) const;

    void BindText(const TextBinding& textBinding);

private:
    vk::DescriptorPool descriptorPool;
    std::unique_ptr<RenderPass> renderPass;

    std::vector<vk::Framebuffer> framebuffers;

    std::vector<TextBinding> textBindings;

    void BuildFrame(Scene* scene) const;
    void AddAnimationSection(Scene* scene) const;

    void HandleResizeEvent(const vk::Extent2D& extent);
};
