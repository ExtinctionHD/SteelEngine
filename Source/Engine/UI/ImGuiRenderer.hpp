#pragma once

class Scene;
class Window;
class RenderPass;
class ImGuiWidget;
struct KeyInput;

class ImGuiRenderer
{
public:
    ImGuiRenderer(const Window& window);
    ~ImGuiRenderer();

    void Build(Scene* scene, float deltaSeconds) const;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

private:
    bool renderingSuspended = false;

    vk::DescriptorPool descriptorPool;
    std::unique_ptr<RenderPass> renderPass;
    std::vector<vk::Framebuffer> framebuffers;

    std::vector<std::unique_ptr<ImGuiWidget>> widgets;

    void HandleResizeEvent(const vk::Extent2D& extent);

    void HandleKeyInputEvent(const KeyInput& keyInput);
};
