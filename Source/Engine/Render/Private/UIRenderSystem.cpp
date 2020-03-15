#include <imgui.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_vulkan.h>

#include "Engine/Window.hpp"
#include "Engine/Render/UIRenderSystem.hpp"

namespace SUIRenderSystem
{
    vk::DescriptorPool CreateDescriptorPool(vk::Device device)
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

        const auto [result, descriptorPool] = device.createDescriptorPool(createInfo);
        Assert(result == vk::Result::eSuccess);

        return descriptorPool;
    }

    std::unique_ptr<RenderPass> CreateRenderPass(const VulkanContext &vulkanContext)
    {
        const AttachmentDescription attachmentDescription{
            AttachmentDescription::Usage::eColor,
            vulkanContext.swapchain->GetFormat(),
            vk::AttachmentLoadOp::eLoad,
            vk::AttachmentStoreOp::eStore,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR
        };

        const RenderPassDescription description{
            vk::PipelineBindPoint::eGraphics, vk::SampleCountFlagBits::e1, { attachmentDescription }
        };

        const PipelineBarrier pipelineBarrier{
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eColorAttachmentWrite,
        };

        std::unique_ptr<RenderPass> renderPass = RenderPass::Create(vulkanContext.device,
                description, { pipelineBarrier, {} });

        return renderPass;
    }

    void InitializeImGui(const VulkanContext &vulkanContext, GLFWwindow *window,
            vk::DescriptorPool descriptorPool, vk::RenderPass renderPass)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsClassic();

        ImGui_ImplGlfw_InitForVulkan(window, true);

        const uint32_t imageCount = static_cast<uint32_t>(vulkanContext.swapchain->GetImages().size());

        ImGui_ImplVulkan_InitInfo initInfo{
            vulkanContext.instance->Get(),
            vulkanContext.device->GetPhysicalDevice(),
            vulkanContext.device->Get(),
            vulkanContext.device->GetQueueProperties().graphicsFamilyIndex,
            vulkanContext.device->GetQueues().graphics,
            nullptr, descriptorPool, imageCount, imageCount,
            VK_SAMPLE_COUNT_1_BIT, nullptr,
            [](VkResult result) { Assert(result == VK_SUCCESS); }
        };

        ImGui_ImplVulkan_Init(&initInfo, renderPass);

        vulkanContext.device->ExecuteOneTimeCommands([](vk::CommandBuffer commandBuffer)
            {
                ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
            });
    }
}

UIRenderSystem::UIRenderSystem(std::shared_ptr<VulkanContext> vulkanContext_, const Window &window)
    : vulkanContext(vulkanContext_)
{
    descriptorPool = SUIRenderSystem::CreateDescriptorPool(vulkanContext->device->Get());
    renderPass = SUIRenderSystem::CreateRenderPass(GetRef(vulkanContext));
    framebuffers = VulkanHelpers::CreateSwapchainFramebuffers(GetRef(vulkanContext->device),
            GetRef(vulkanContext->swapchain), GetRef(renderPass));

    SUIRenderSystem::InitializeImGui(GetRef(vulkanContext), window.Get(), descriptorPool, renderPass->Get());
}

UIRenderSystem::~UIRenderSystem()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    for (auto &framebuffer : framebuffers)
    {
        vulkanContext->device->Get().destroyFramebuffer(framebuffer);
    }

    vulkanContext->device->Get().destroyDescriptorPool(descriptorPool);
}

void UIRenderSystem::Process(float)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static bool show_demo_window;
    static bool show_another_window;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
    {
        ImGui::ShowDemoWindow(&show_demo_window);
    }

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text."); // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", reinterpret_cast<float *>(&clear_color)); // Edit 3 floats representing a color

        // Buttons return true when clicked
        if (ImGui::Button("Button"))
        {
            counter++;
        }

        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
        {
            show_another_window = false;
        }
        ImGui::End();
    }

    ImGui::Render();
}

void UIRenderSystem::OnResize(const vk::Extent2D &extent)
{
    if (extent.width == 0 || extent.height == 0)
    {
        return;
    }

    for (auto &framebuffer : framebuffers)
    {
        vulkanContext->device->Get().destroyFramebuffer(framebuffer);
    }

    const uint32_t imageCount = static_cast<uint32_t>(vulkanContext->swapchain->GetImages().size());
    ImGui_ImplVulkan_SetMinImageCount(imageCount);

    renderPass = SUIRenderSystem::CreateRenderPass(GetRef(vulkanContext));
    framebuffers = VulkanHelpers::CreateSwapchainFramebuffers(GetRef(vulkanContext->device),
            GetRef(vulkanContext->swapchain), GetRef(renderPass));
}

RenderFunction UIRenderSystem::GetUIRenderFunction()
{
    return [this](vk::CommandBuffer commandBuffer, uint32_t imageIndex)
        {
            RenderUI(commandBuffer, imageIndex);
        };
}

void UIRenderSystem::RenderUI(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    const vk::Extent2D &extent = vulkanContext->swapchain->GetExtent();

    const vk::Rect2D renderArea(vk::Offset2D(0, 0), extent);

    const vk::RenderPassBeginInfo beginInfo(renderPass->Get(), framebuffers[imageIndex], renderArea);

    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    commandBuffer.endRenderPass();
}
