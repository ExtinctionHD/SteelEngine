#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

class FrameLoop
{
public:
    FrameLoop();
    ~FrameLoop();

    uint32_t GetFrameCount() const;

    bool IsFrameActive(uint32_t index) const;

    void Draw(RenderCommands renderCommands);

    void DestroyResource(std::function<void()>&& destroyTask);

private:
    struct Frame
    {
        vk::CommandBuffer commandBuffer;
        CommandBufferSync commandBufferSync;
    };

    struct ResourceToDestroy
    {
        std::function<void()> destroyTask;
        std::set<uint32_t> framesToWait;
    };

    uint32_t currentFrameIndex = 0;
    std::vector<Frame> frames;

    std::vector<ResourceToDestroy> resourcesToDestroy;

    void UpdateResourcesToDestroy();
};
