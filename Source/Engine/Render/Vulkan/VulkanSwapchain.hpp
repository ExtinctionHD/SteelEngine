#pragma once

#include "Engine/Render/Vulkan/VulkanDevice.hpp"

class Window;

class VulkanSwapchain
{
public:
    static std::unique_ptr<VulkanSwapchain> Create(std::shared_ptr<VulkanDevice> device,
        vk::SurfaceKHR surface, const Window &window);

    VulkanSwapchain(std::shared_ptr<VulkanDevice> aDevice, vk::SwapchainKHR aSwapchain,
        const std::vector<vk::ImageView> &aImageViews);
    ~VulkanSwapchain();

    vk::SwapchainKHR Get() const { return swapchain; }

private:
    std::shared_ptr<VulkanDevice> device;

    vk::SwapchainKHR swapchain;

    std::vector<vk::ImageView> imageViews;
};
