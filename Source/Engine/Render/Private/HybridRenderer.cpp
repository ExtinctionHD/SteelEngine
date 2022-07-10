#include "Engine/Render/HybridRenderer.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Engine.hpp"
#include "Engine/EngineHelpers.hpp"
#include "Engine/InputHelpers.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Stages/ForwardStage.hpp"
#include "Engine/Render/Stages/GBufferStage.hpp"
#include "Engine/Render/Stages/LightingStage.hpp"

namespace Details
{
    const LightVolume* GetLightVolumeIfPresent(const LightVolume& lightVolume)
    {
        if constexpr (Config::kGlobalIllumination)
        {
            return &lightVolume;
        }
        else
        {
            return nullptr;
        }
    }
}

HybridRenderer::HybridRenderer(const Scene* scene_)
    : scene(scene_)
{
    if constexpr (Config::kGlobalIllumination)
    {
        lightVolume = RenderContext::globalIllumination->GenerateLightVolume(scene);
    }

    SetupGBufferTextures();

    SetupRenderStages();

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &HybridRenderer::HandleResizeEvent));

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &HybridRenderer::HandleKeyInputEvent));
}

HybridRenderer::~HybridRenderer()
{
    if constexpr (Config::kGlobalIllumination)
    {
        VulkanContext::bufferManager->DestroyBuffer(lightVolume.positionsBuffer);
        VulkanContext::bufferManager->DestroyBuffer(lightVolume.tetrahedralBuffer);
        VulkanContext::bufferManager->DestroyBuffer(lightVolume.coefficientsBuffer);
    }

    for (const auto& texture : gBufferTextures)
    {
        VulkanContext::textureManager->DestroyTexture(texture);
    }
}

void HybridRenderer::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    gBufferStage->Execute(commandBuffer, imageIndex);

    lightingStage->Execute(commandBuffer, imageIndex);

    forwardStage->Execute(commandBuffer, imageIndex);
}

void HybridRenderer::SetupGBufferTextures()
{
    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    gBufferTextures.resize(GBufferStage::kFormats.size());

    for (size_t i = 0; i < gBufferTextures.size(); ++i)
    {
        constexpr vk::ImageUsageFlags colorImageUsage
                = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage;

        constexpr vk::ImageUsageFlags depthImageUsage
                = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;

        const vk::Format format = GBufferStage::kFormats[i];

        const vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;

        const vk::ImageUsageFlags imageUsage = ImageHelpers::IsDepthFormat(format)
                ? depthImageUsage : colorImageUsage;

        gBufferTextures[i] = ImageHelpers::CreateRenderTarget(
                format, extent, sampleCount, imageUsage);
    }

    VulkanContext::device->ExecuteOneTimeCommands([this](vk::CommandBuffer commandBuffer)
        {
            const ImageLayoutTransition colorLayoutTransition{
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eGeneral,
                PipelineBarrier::kEmpty
            };

            const ImageLayoutTransition depthLayoutTransition{
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eDepthStencilAttachmentOptimal,
                PipelineBarrier::kEmpty
            };

            for (size_t i = 0; i < gBufferTextures.size(); ++i)
            {
                const vk::Image image = gBufferTextures[i].image;

                const vk::Format format = GBufferStage::kFormats[i];

                const vk::ImageSubresourceRange& subresourceRange = ImageHelpers::IsDepthFormat(format)
                        ? ImageHelpers::kFlatDepth : ImageHelpers::kFlatColor;

                const ImageLayoutTransition& layoutTransition = ImageHelpers::IsDepthFormat(format)
                        ? depthLayoutTransition : colorLayoutTransition;

                ImageHelpers::TransitImageLayout(commandBuffer, image, subresourceRange, layoutTransition);
            }
        });
}

void HybridRenderer::SetupRenderStages()
{
    const std::vector<vk::ImageView> gBufferImageViews = TextureHelpers::GetViews(gBufferTextures);

    gBufferStage = std::make_unique<GBufferStage>(scene, gBufferImageViews);

    lightingStage = std::make_unique<LightingStage>(scene, &lightVolume, gBufferImageViews);

    forwardStage = std::make_unique<ForwardStage>(scene, &lightVolume, gBufferImageViews.back());
}

void HybridRenderer::HandleResizeEvent(const vk::Extent2D& extent)
{
    if (extent.width != 0 && extent.height != 0)
    {
        for (const auto& texture : gBufferTextures)
        {
            VulkanContext::textureManager->DestroyTexture(texture);
        }

        SetupGBufferTextures();

        const std::vector<vk::ImageView> gBufferImageViews = TextureHelpers::GetViews(gBufferTextures);

        gBufferStage->Resize(gBufferImageViews);

        lightingStage->Resize(gBufferImageViews);

        forwardStage->Resize(gBufferImageViews.back());
    }
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
    VulkanContext::device->WaitIdle();

    gBufferStage->ReloadShaders();

    lightingStage->ReloadShaders();

    forwardStage->ReloadShaders();
}
