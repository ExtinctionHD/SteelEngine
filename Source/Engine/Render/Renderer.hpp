#pragma once

class Renderer
{
public:
    static const vk::ImageLayout kFinalLayout = vk::ImageLayout::eColorAttachmentOptimal;

    Renderer() = default;

    virtual ~Renderer() = default;

    virtual void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

    virtual void HandleResizeEvent(const vk::Extent2D &extent);
};

inline void Renderer::Render(vk::CommandBuffer, uint32_t) {}

inline void Renderer::HandleResizeEvent(const vk::Extent2D &) {}
