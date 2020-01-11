#pragma once

#include "Engine/Render/Vulkan/Device.hpp"

class Window;

class Swapchain
{
public:
    static std::unique_ptr<Swapchain> Create(std::shared_ptr<Device> device,
            vk::SurfaceKHR surface, const Window &window);

    Swapchain(std::shared_ptr<Device> aDevice, vk::SwapchainKHR aSwapchain,
            vk::Format aFormat, const std::vector<vk::ImageView> &aImageViews);
    ~Swapchain();

    vk::SwapchainKHR Get() const { return swapchain; }

    vk::Format GetFormat() const { return format; }

private:
    std::shared_ptr<Device> device;

    vk::SwapchainKHR swapchain;

    vk::Format format;

    std::vector<vk::ImageView> imageViews;
};
