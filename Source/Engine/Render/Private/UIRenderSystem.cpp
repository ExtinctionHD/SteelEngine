#include <imgui.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_vulkan.h>

#include "Engine/Render/UIRenderSystem.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Window.hpp"
#include "Engine/Engine.hpp"

namespace SUIRenderSystem
{
    vk::DescriptorPool CreateDescriptorPool()
    {
        const std::vector<vk::DescriptorPoolSize> descriptorPoolSizes{
            { vk::DescriptorType::eSampler, 1024 },
            { vk::DescriptorType::eCombinedImageSampler, 1024 },
            { vk::DescriptorType::eSampledImage, 1024 },
            { vk::DescriptorType::eStorageImage, 1024 },
            { vk::DescriptorType::eUniformTexelBuffer, 1024 },
            { vk::DescriptorType::eStorageTexelBuffer, 1024 },
            { vk::DescriptorType::eUniformBuffer, 1024 },
            { vk::DescriptorType::eStorageBuffer, 1024 },
            { vk::DescriptorType::eUniformBufferDynamic, 1024 },
            { vk::DescriptorType::eStorageBufferDynamic, 1024 },
            { vk::DescriptorType::eInputAttachment, 1024 },
        };

        const uint32_t maxDescriptorSetCount = 1024;

        const vk::DescriptorPoolCreateInfo createInfo(
                vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, maxDescriptorSetCount,
                static_cast<uint32_t>(descriptorPoolSizes.size()), descriptorPoolSizes.data());

        const auto [result, descriptorPool] = VulkanContext::device->Get().createDescriptorPool(createInfo);
        Assert(result == vk::Result::eSuccess);

        return descriptorPool;
    }

    std::unique_ptr<RenderPass> CreateRenderPass()
    {
        const RenderPass::Attachment attachmentDescription{
            RenderPass::Attachment::Usage::eColor,
            VulkanContext::swapchain->GetFormat(),
            vk::AttachmentLoadOp::eLoad,
            vk::AttachmentStoreOp::eStore,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR
        };

        const RenderPass::Description description{
            vk::PipelineBindPoint::eGraphics, vk::SampleCountFlagBits::e1, { attachmentDescription }
        };

        const PipelineBarrier pipelineBarrier{
            SyncScope::kColorAttachmentWrite, SyncScope::kColorAttachmentWrite
        };

        std::unique_ptr<RenderPass> renderPass = RenderPass::Create(description,
                RenderPass::Dependencies{ pipelineBarrier, std::nullopt });

        return renderPass;
    }

    void InitializeImGui(GLFWwindow *window, vk::DescriptorPool descriptorPool, vk::RenderPass renderPass)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsClassic();

        ImGui_ImplGlfw_InitForVulkan(window, true);

        const uint32_t imageCount = static_cast<uint32_t>(VulkanContext::swapchain->GetImages().size());

        ImGui_ImplVulkan_InitInfo initInfo{
            VulkanContext::instance->Get(),
            VulkanContext::device->GetPhysicalDevice(),
            VulkanContext::device->Get(),
            VulkanContext::device->GetQueuesDescription().graphicsFamilyIndex,
            VulkanContext::device->GetQueues().graphics,
            nullptr, descriptorPool, imageCount, imageCount,
            VK_SAMPLE_COUNT_1_BIT, nullptr,
            [](VkResult result) { Assert(result == VK_SUCCESS); }
        };

        ImGui_ImplVulkan_Init(&initInfo, renderPass);

        VulkanContext::device->ExecuteOneTimeCommands([](vk::CommandBuffer commandBuffer)
            {
                ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
            });
    }
}

UIRenderSystem::UIRenderSystem(const Window &window)
{
    descriptorPool = SUIRenderSystem::CreateDescriptorPool();
    renderPass = SUIRenderSystem::CreateRenderPass();
    framebuffers = VulkanHelpers::CreateSwapchainFramebuffers(VulkanContext::device->Get(), renderPass->Get(),
            VulkanContext::swapchain->GetExtent(), VulkanContext::swapchain->GetImageViews(), {});

    SUIRenderSystem::InitializeImGui(window.Get(), descriptorPool, renderPass->Get());

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &UIRenderSystem::HandleResizeEvent));
}

UIRenderSystem::~UIRenderSystem()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    for (auto &framebuffer : framebuffers)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }

    VulkanContext::device->Get().destroyDescriptorPool(descriptorPool);
}

void UIRenderSystem::Process(float)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const float fps = ImGui::GetIO().Framerate;

    ImGui::Begin("Steel Engine");
    ImGui::Text("Frame time: %.2f ms/frame (%.1f FPS)", 1000.0f / fps, fps);
    ImGui::End();

    ImGui::Render();
}

void UIRenderSystem::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    const vk::Extent2D &extent = VulkanContext::swapchain->GetExtent();

    const vk::Rect2D renderArea(vk::Offset2D(0, 0), extent);

    const vk::RenderPassBeginInfo beginInfo(renderPass->Get(), framebuffers[imageIndex], renderArea);

    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    commandBuffer.endRenderPass();
}

void UIRenderSystem::HandleResizeEvent(const vk::Extent2D &extent)
{
    if (extent.width == 0 || extent.height == 0)
    {
        return;
    }

    for (auto &framebuffer : framebuffers)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }

    const uint32_t imageCount = static_cast<uint32_t>(VulkanContext::swapchain->GetImages().size());
    ImGui_ImplVulkan_SetMinImageCount(imageCount);

    renderPass = SUIRenderSystem::CreateRenderPass();
    framebuffers = VulkanHelpers::CreateSwapchainFramebuffers(VulkanContext::device->Get(), renderPass->Get(),
            VulkanContext::swapchain->GetExtent(), VulkanContext::swapchain->GetImageViews(), {});
}
