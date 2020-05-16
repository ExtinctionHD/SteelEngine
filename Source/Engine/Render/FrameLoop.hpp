#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

class FrameLoop
{
public:
    FrameLoop();
    ~FrameLoop();

    void Draw(RenderCommands renderCommands);

private:
    struct Frame
    {
        vk::CommandBuffer commandBuffer;
        CommandBufferSync sync;
    };

    uint32_t frameIndex = 0;
    std::vector<Frame> frames;
};
