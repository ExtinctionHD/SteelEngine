#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "Engine/UI/ImGuiRenderer.hpp"

#include "Engine/UI/AnimationWidget.hpp"
#include "Engine/UI/HierarchyWidget.hpp"
#include "Engine/UI/EntityWidget.hpp"
#include "Engine/UI/StatWidget.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Window.hpp"
#include "Engine/Engine.hpp"

namespace Details
{
    static const Filepath kFontPath("~/Assets/Fonts/Roboto-Medium.ttf");

    static constexpr float kFontSize = 16.0f;

    static vk::DescriptorPool CreateDescriptorPool()
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

    static std::unique_ptr<RenderPass> CreateRenderPass()
    {
        const RenderPass::AttachmentDescription attachmentDescription{
            RenderPass::AttachmentUsage::eColor,
            VulkanContext::swapchain->GetFormat(),
            vk::AttachmentLoadOp::eLoad,
            vk::AttachmentStoreOp::eStore,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR
        };

        const RenderPass::Description description{
            vk::PipelineBindPoint::eGraphics,
            vk::SampleCountFlagBits::e1,
            { attachmentDescription }
        };

        const PipelineBarrier previousDependency{
            SyncScope::kColorAttachmentWrite,
            SyncScope::kColorAttachmentRead
        };

        std::unique_ptr<RenderPass> renderPass = RenderPass::Create(description,
                RenderPass::Dependencies{ { previousDependency }, {} });

        return renderPass;
    }

    static std::vector<vk::Framebuffer> CreateFramebuffers(const RenderPass& renderPass)
    {
        const vk::Device device = VulkanContext::device->Get();
        const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();
        const std::vector<vk::ImageView>& imageViews = VulkanContext::swapchain->GetImageViews();

        return VulkanHelpers::CreateFramebuffers(device, renderPass.Get(), extent, { imageViews }, {});
    }

    static void InitializeImGui(GLFWwindow* window, vk::DescriptorPool descriptorPool, vk::RenderPass renderPass)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsClassic();

        ImGui::GetIO().Fonts->AddFontFromFileTTF(kFontPath.GetAbsolute().c_str(), kFontSize);

        ImGui_ImplGlfw_InitForVulkan(window, true);

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance = VulkanContext::instance->Get();
        initInfo.PhysicalDevice = VulkanContext::device->GetPhysicalDevice();
        initInfo.Device = VulkanContext::device->Get();
        initInfo.QueueFamily = VulkanContext::device->GetQueuesDescription().graphicsFamilyIndex;
        initInfo.Queue = VulkanContext::device->GetQueues().graphics;
        initInfo.DescriptorPool = descriptorPool;
        initInfo.MinImageCount = VulkanContext::swapchain->GetImageCount();
        initInfo.ImageCount = VulkanContext::swapchain->GetImageCount();
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.CheckVkResultFn = [](VkResult result) { Assert(result == VK_SUCCESS); };

        ImGui_ImplVulkan_Init(&initInfo, renderPass);

        VulkanContext::device->ExecuteOneTimeCommands([](vk::CommandBuffer commandBuffer)
            {
                ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
            });
    }

    static void SetNextWindowProperties()
    {
        const auto& [width, height] = VulkanContext::swapchain->GetExtent();
        
        ImGui::SetNextWindowPos(ImVec2(0, 0));

        const ImVec2 minSize = ImVec2(static_cast<float>(width) * 0.1f, static_cast<float>(height));
        const ImVec2 maxSize = ImVec2(static_cast<float>(width) * 0.5f, static_cast<float>(height));

        ImGui::SetNextWindowSizeConstraints(minSize, maxSize);
    }
}

ImGuiRenderer::ImGuiRenderer(const Window& window)
{
    EASY_FUNCTION()

    descriptorPool = Details::CreateDescriptorPool();
    renderPass = Details::CreateRenderPass();
    framebuffers = Details::CreateFramebuffers(*renderPass);

    Details::InitializeImGui(window.Get(), descriptorPool, renderPass->Get());

    widgets.push_back(std::make_unique<StatWidget>());
    widgets.push_back(std::make_unique<HierarchyWidget>());
    widgets.push_back(std::make_unique<EntityWidget>());
    widgets.push_back(std::make_unique<AnimationWidget>());

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &ImGuiRenderer::HandleResizeEvent));
}

ImGuiRenderer::~ImGuiRenderer()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    for (const auto& framebuffer : framebuffers)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }

    VulkanContext::device->Get().destroyDescriptorPool(descriptorPool);
}

void ImGuiRenderer::Build(Scene* scene, float deltaSeconds) const
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    Details::SetNextWindowProperties();

    ImGui::Begin("Debug Overlay", nullptr, ImGuiWindowFlags_NoMove);

    for (const auto& widget : widgets)
    {
        widget->Build(scene, deltaSeconds);
    }

    ImGui::End();
}

void ImGuiRenderer::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    ImGui::Render();

    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    const vk::Rect2D renderArea(vk::Offset2D(), extent);

    const vk::RenderPassBeginInfo beginInfo(renderPass->Get(), framebuffers[imageIndex], renderArea);

    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    commandBuffer.endRenderPass();
}

void ImGuiRenderer::HandleResizeEvent(const vk::Extent2D& extent)
{
    if (extent.width != 0 && extent.height != 0)
    {
        for (const auto& framebuffer : framebuffers)
        {
            VulkanContext::device->Get().destroyFramebuffer(framebuffer);
        }

        const uint32_t imageCount = static_cast<uint32_t>(VulkanContext::swapchain->GetImages().size());
        ImGui_ImplVulkan_SetMinImageCount(imageCount);

        renderPass = Details::CreateRenderPass();
        framebuffers = Details::CreateFramebuffers(*renderPass);
    }
}
