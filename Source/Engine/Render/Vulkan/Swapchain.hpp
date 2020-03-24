#pragma once

class Window;

struct SwapchainDescription
{
    vk::Extent2D surfaceExtent;
    bool vSyncEnabled = false;
};

class Swapchain
{
public:
    static std::unique_ptr<Swapchain> Create(const SwapchainDescription &description);
    ~Swapchain();

    vk::SwapchainKHR Get() const { return swapchain; }

    vk::Format GetFormat() const { return format; }

    const vk::Extent2D &GetExtent() const { return extent; }

    const std::vector<vk::Image> &GetImages() const { return images; }

    const std::vector<vk::ImageView> &GetImageViews() const { return imageViews; }

    void Recreate(const SwapchainDescription &description);

private:
    vk::SwapchainKHR swapchain;

    vk::Format format;

    vk::Extent2D extent;

    std::vector<vk::Image> images;
    std::vector<vk::ImageView> imageViews;

    Swapchain(vk::SwapchainKHR swapchain_, vk::Format format_, const vk::Extent2D &extent_);

    void Destroy();
};
