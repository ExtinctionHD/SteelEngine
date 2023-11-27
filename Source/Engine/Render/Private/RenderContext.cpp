#include "Engine/Render/RenderContext.hpp"

#include "Engine/Render/FrameLoop.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/ImageBasedLighting.hpp"
#include "Engine/Scene/GlobalIllumination.hpp"

std::unique_ptr<FrameLoop> RenderContext::frameLoop;
std::unique_ptr<ImageBasedLighting> RenderContext::imageBasedLighting;
std::unique_ptr<GlobalIllumination> RenderContext::globalIllumination;

void RenderContext::Create()
{
    EASY_FUNCTION()

    frameLoop = std::make_unique<FrameLoop>();
    imageBasedLighting = std::make_unique<ImageBasedLighting>();
    globalIllumination = std::make_unique<GlobalIllumination>();
}

void RenderContext::Destroy()
{
    imageBasedLighting.reset();
    globalIllumination.reset();
    frameLoop.reset();
}
