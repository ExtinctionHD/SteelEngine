#include "Engine/Render/Vulkan/Swapchain.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Assert.hpp"

namespace SSwapchain
{
    vk::Format SelectFormat(const std::vector<vk::SurfaceFormatKHR> &formats,
            const std::vector<vk::Format> &preferredFormats)
    {
        Assert(!formats.empty());

        Assert(!preferredFormats.empty());

        if (formats.front().format == vk::Format::eUndefined)
        {
            if (preferredFormats.front() != vk::Format::eUndefined)
            {
                return preferredFormats.front();
            }
        }

        for (const auto &format : preferredFormats)
        {
            const auto it = std::find_if(formats.begin(), formats.end(),
                    [format](const auto &entry) { return entry.format == format; });

            if (it != formats.end())
            {
                return format;
            }
        }

        return vk::Format::eB8G8R8A8Unorm;
    }

    vk::Extent2D SelectExtent(const vk::SurfaceCapabilitiesKHR &capabilities,
            const vk::Extent2D &requiredExtent)
    {
        vk::Extent2D swapchainExtent;

        if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
        {
            swapchainExtent.width = std::clamp(requiredExtent.width,
                    capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            swapchainExtent.height = std::clamp(requiredExtent.height,
                    capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }
        else
        {
            swapchainExtent = capabilities.currentExtent;
        }

        return swapchainExtent;
    }

    vk::SharingMode SelectSharingMode(const std::vector<uint32_t> &uniqueQueueFamilyIndices)
    {
        if (uniqueQueueFamilyIndices.size() == 1)
        {
            return vk::SharingMode::eExclusive;
        }

        return vk::SharingMode::eConcurrent;
    }

    vk::SurfaceTransformFlagBitsKHR SelectPreTransform(vk::SurfaceCapabilitiesKHR capabilities)
    {
        if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
        {
            return vk::SurfaceTransformFlagBitsKHR::eIdentity;
        }

        return capabilities.currentTransform;
    }

    vk::CompositeAlphaFlagBitsKHR SelectCompositeAlpha(vk::SurfaceCapabilitiesKHR capabilities)
    {
        std::vector<vk::CompositeAlphaFlagBitsKHR> compositeAlphaFlagBits{
            vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
            vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
            vk::CompositeAlphaFlagBitsKHR::eInherit
        };

        for (const auto &compositeAlpha : compositeAlphaFlagBits)
        {
            if (capabilities.supportedCompositeAlpha & compositeAlpha)
            {
                return compositeAlpha;
            }
        }

        return vk::CompositeAlphaFlagBitsKHR::eOpaque;
    }

    std::vector<vk::Image> RetrieveImages(vk::Device device, vk::SwapchainKHR swapchain)
    {
        const auto [result, images] = device.getSwapchainImagesKHR(swapchain);
        Assert(result == vk::Result::eSuccess);

        return images;
    }

    std::vector<vk::ImageView> CreateImageViews(vk::Device device,
            const std::vector<vk::Image> &images, vk::Format format)
    {
        std::vector<vk::ImageView> imageViews;
        imageViews.reserve(images.size());

        for (const auto &image : images)
        {
            vk::ImageViewCreateInfo createInfo({},
                    image, vk::ImageViewType::e2D, format,
                    VulkanHelpers::kComponentMappingRgba,
                    VulkanHelpers::kSubresourceRangeColor);

            const auto [res, imageView] = device.createImageView(createInfo);
            Assert(res == vk::Result::eSuccess);

            imageViews.push_back(imageView);
        }

        return imageViews;
    }
}

std::unique_ptr<Swapchain> Swapchain::Create(std::shared_ptr<Device> device,
        vk::SurfaceKHR surface, const vk::Extent2D &surfaceExtent, const std::vector<vk::Format> &formats)
{
    const auto capabilities = device->GetSurfaceCapabilities(surface);

    const std::vector<uint32_t> uniqueQueueFamilyIndices = device->GetQueueProperties().GetUniqueIndices();

    const vk::SurfaceFormatKHR format = SSwapchain::SelectFormat(device->GetSurfaceFormats(surface), formats);
    const vk::Extent2D extent = SSwapchain::SelectExtent(capabilities, surfaceExtent);

    const vk::SwapchainCreateInfoKHR createInfo({}, surface,
            capabilities.minImageCount, format.format, format.colorSpace,
            extent, 1, vk::ImageUsageFlagBits::eColorAttachment,
            SSwapchain::SelectSharingMode(uniqueQueueFamilyIndices),
            static_cast<uint32_t>(uniqueQueueFamilyIndices.size()), uniqueQueueFamilyIndices.data(),
            SSwapchain::SelectPreTransform(capabilities),
            SSwapchain::SelectCompositeAlpha(capabilities),
            vk::PresentModeKHR::eFifo, false, nullptr);

    const auto [result, swapchain] = device->Get().createSwapchainKHR(createInfo);
    Assert(result == vk::Result::eSuccess);

    LogD << "Swapchain created" << "\n";

    return std::unique_ptr<Swapchain>(new Swapchain(device, swapchain, format.format, extent));
}

Swapchain::Swapchain(std::shared_ptr<Device> aDevice, vk::SwapchainKHR aSwapchain,
        vk::Format aFormat, const vk::Extent2D &aExtent)
    : device(aDevice)
    , swapchain(aSwapchain)
    , format(aFormat)
    , extent(aExtent)
{
    images = SSwapchain::RetrieveImages(device->Get(), swapchain);
    imageViews = SSwapchain::CreateImageViews(device->Get(), images, format);
}

Swapchain::~Swapchain()
{
    for (const auto &imageView : imageViews)
    {
        device->Get().destroyImageView(imageView);
    }

    device->Get().destroySwapchainKHR(swapchain);
}
