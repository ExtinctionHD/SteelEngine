#pragma once

#include "Engine/Render/Vulkan/Device.hpp"

class Window;

struct SwapchainDescription
{
    vk::SurfaceKHR surface;
    vk::Extent2D surfaceExtent;
    bool vSyncEnabled = false;
};

class Swapchain
{
public:
    static std::unique_ptr<Swapchain> Create(std::shared_ptr<Device> device, const SwapchainDescription &description);
    ~Swapchain();

    vk::SwapchainKHR Get() const { return swapchain; }

    vk::Format GetFormat() const { return format; }

    const vk::Extent2D &GetExtent() const { return extent; }

    const std::vector<vk::Image> &GetImages() const { return images; }

    const std::vector<vk::ImageView> &GetImageViews() const { return imageViews; }

    void Recreate(const SwapchainDescription &surfaceInfo);

private:
    std::shared_ptr<Device> device;

    vk::SwapchainKHR swapchain;

    vk::Format format;

    vk::Extent2D extent;

    std::vector<vk::Image> images;
    std::vector<vk::ImageView> imageViews;

    Swapchain(std::shared_ptr<Device> device_, vk::SwapchainKHR swapchain_,
            vk::Format format_, const vk::Extent2D &extent_);

    void Destroy();
};
