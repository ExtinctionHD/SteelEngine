#include "Engine/System/RenderSystem.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/EngineHelpers.hpp"
#include "Engine/InputHelpers.hpp"

namespace Details
{
}

RenderSystem::RenderSystem(Scene* scene_)
    : scene(scene_)
{
}

RenderSystem::~RenderSystem()
{
}

void RenderSystem::Process(float) {}

void RenderSystem::Render(vk::CommandBuffer , uint32_t )
{
}

void RenderSystem::HandleResizeEvent(const vk::Extent2D& extent)
{
    if (extent.width != 0 && extent.height != 0)
    {
    }
}

void RenderSystem::HandleKeyInputEvent(const KeyInput& keyInput)
{
    if (keyInput.action == KeyAction::ePress)
    {
        switch (keyInput.key)
        {
        case Key::eR:
            ReloadShaders();
            break;
        default:
            break;
        }
    }
}

void RenderSystem::ReloadShaders()
{
    VulkanContext::device->WaitIdle();
}
