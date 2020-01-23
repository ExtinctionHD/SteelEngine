#pragma once

#include "Engine/Render/Vulkan/Device.hpp"

class Window;

class Swapchain
{
public:
    static std::unique_ptr<Swapchain> Create(std::shared_ptr<Device> device,
            vk::SurfaceKHR surface, const Window &window);

    Swapchain(std::shared_ptr<Device> aDevice, vk::SwapchainKHR aSwapchain, vk::Format aFormat,
            const vk::Extent2D &aExtent, const std::vector<vk::ImageView> &aImageViews);
    ~Swapchain();

    vk::SwapchainKHR Get() const { return swapchain; }

    vk::Format GetFormat() const { return format; }

    const vk::Extent2D &GetExtent() const { return extent; }

    const std::vector<vk::ImageView> &GetImageViews() const;

private:
    std::shared_ptr<Device> device;

    vk::SwapchainKHR swapchain;

    vk::Format format;

    vk::Extent2D extent;

    std::vector<vk::ImageView> imageViews;
};
