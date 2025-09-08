#include "Engine/Render/RenderContext.hpp"

#include "Engine/Render/FrameLoop.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/ImageBasedLighting.hpp"

std::unique_ptr<FrameLoop> RenderContext::frameLoop;
std::unique_ptr<ImageBasedLighting> RenderContext::imageBasedLighting;

void RenderContext::Create()
{
    EASY_FUNCTION()

    frameLoop = std::make_unique<FrameLoop>();
    imageBasedLighting = std::make_unique<ImageBasedLighting>();
}

void RenderContext::Destroy()
{
    imageBasedLighting.reset();
    frameLoop.reset();
}
