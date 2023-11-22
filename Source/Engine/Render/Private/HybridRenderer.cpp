#include "Engine/Render/HybridRenderer.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Engine.hpp"
#include "Engine/EngineHelpers.hpp"
#include "Engine/InputHelpers.hpp"
#include "Engine/Render/Stages/ForwardStage.hpp"
#include "Engine/Render/Stages/GBufferStage.hpp"
#include "Engine/Render/Stages/LightingStage.hpp"

namespace Details
{
    void TransitSwapchainImageLayout(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
    {
        const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];

        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::ePresentSrcKHR,
            vk::ImageLayout::eColorAttachmentOptimal,
            PipelineBarrier{
                SyncScope::kWaitForNone,
                SyncScope::kColorAttachmentWrite
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, swapchainImage,
                ImageHelpers::kFlatColor, layoutTransition);
    }
}

HybridRenderer::HybridRenderer()
{
    EASY_FUNCTION()

    gBufferStage = std::make_unique<GBufferStage>();
    lightingStage = std::make_unique<LightingStage>(gBufferStage->GetRenderTargets());
    forwardStage = std::make_unique<ForwardStage>(gBufferStage->GetDepthTarget());

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &HybridRenderer::HandleKeyInputEvent));
}

HybridRenderer::~HybridRenderer() = default;

void HybridRenderer::RegisterScene(const Scene* scene_)
{
    EASY_FUNCTION()

    RemoveScene();

    scene = scene_;

    gBufferStage->RegisterScene(scene);
    lightingStage->RegisterScene(scene);
    forwardStage->RegisterScene(scene);
}

void HybridRenderer::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    gBufferStage->RemoveScene();
    lightingStage->RemoveScene();
    forwardStage->RemoveScene();

    scene = nullptr;
}

void HybridRenderer::Update() const
{
    gBufferStage->Update();
    lightingStage->Update();
    forwardStage->Update();
}

void HybridRenderer::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    if (scene)
    {
        gBufferStage->Execute(commandBuffer, imageIndex);
        lightingStage->Execute(commandBuffer, imageIndex);
        forwardStage->Execute(commandBuffer, imageIndex);
    }
    else
    {
        Details::TransitSwapchainImageLayout(commandBuffer, imageIndex);
    }
}

void HybridRenderer::Resize(const vk::Extent2D& extent) const
{
    Assert(extent.width != 0 && extent.height != 0);

    gBufferStage->Resize();
    lightingStage->Resize(gBufferStage->GetRenderTargets());
    forwardStage->Resize(gBufferStage->GetDepthTarget());
}

void HybridRenderer::HandleKeyInputEvent(const KeyInput& keyInput) const
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

void HybridRenderer::ReloadShaders() const
{
    if (!scene)
    {
        return;
    }

    VulkanContext::device->WaitIdle();

    gBufferStage->ReloadShaders();
    lightingStage->ReloadShaders();
    forwardStage->ReloadShaders();
}
