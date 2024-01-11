#pragma once

class Window;
class RenderPass;
class Scene;

class ImGuiWidget
{
public:
    ImGuiWidget(const std::string& name_);
    virtual ~ImGuiWidget() = default;

    void Update(const Scene* scene, float deltaSeconds) const;

protected:
    virtual void UpdateInternal(const Scene* scene, float deltaSeconds) const = 0;

private:
    std::string name;
};

class ImGuiRenderer
{
public:
    ImGuiRenderer(const Window& window);
    ~ImGuiRenderer();

    void Update(const Scene* scene, float deltaSeconds) const;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

private:
    vk::DescriptorPool descriptorPool;
    std::unique_ptr<RenderPass> renderPass;
    std::vector<vk::Framebuffer> framebuffers;

    std::vector<std::unique_ptr<ImGuiWidget>> widgets;

    void HandleResizeEvent(const vk::Extent2D& extent);
};
