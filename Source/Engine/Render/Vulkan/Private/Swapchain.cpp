#include "Engine/Render/Vulkan/Swapchain.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"

namespace SSwapchain
{
    struct SwapchainData
    {
        vk::SwapchainKHR swapchain;
        vk::Format format;
        vk::Extent2D extent;
    };

    vk::SurfaceFormatKHR SelectFormat(const std::vector<vk::SurfaceFormatKHR> &formats,
            const std::vector<vk::Format> &preferredFormats)
    {
        Assert(!formats.empty());
        Assert(!preferredFormats.empty());

        vk::SurfaceFormatKHR selectedFormat = vk::Format::eUndefined;

        for (const auto &preferredFormat : preferredFormats)
        {
            if (preferredFormat == vk::Format::eUndefined)
            {
                return formats.front();
            }

            const auto it = std::find_if(formats.begin(), formats.end(),
                    [preferredFormat](const auto &entry) { return entry.format == preferredFormat; });

            if (it != formats.end())
            {
                selectedFormat = *it;
            }
        }

        Assert(selectedFormat.format != vk::Format::eUndefined);

        return selectedFormat;
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

    vk::PresentModeKHR SelectPresentMode(bool vSyncEnabled)
    {
        return vSyncEnabled ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox;
    }

    SwapchainData CreateSwapchain(const Device &device, const SwapchainInfo &swapchainInfo)
    {
        const auto &[surface, surfaceExtent, vSyncEnabled] = swapchainInfo;

        const auto capabilities = device.GetSurfaceCapabilities(surface);

        const std::vector<uint32_t> uniqueQueueFamilyIndices = device.GetQueueProperties().GetUniqueIndices();

        const vk::SurfaceFormatKHR format = SelectFormat(device.GetSurfaceFormats(surface), { vk::Format::eUndefined });
        const vk::Extent2D extent = SelectExtent(capabilities, surfaceExtent);

        const vk::SwapchainCreateInfoKHR createInfo({}, surface,
                capabilities.minImageCount, format.format, format.colorSpace,
                extent, 1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage,
                SelectSharingMode(uniqueQueueFamilyIndices),
                static_cast<uint32_t>(uniqueQueueFamilyIndices.size()), uniqueQueueFamilyIndices.data(),
                SelectPreTransform(capabilities),
                SelectCompositeAlpha(capabilities),
                SelectPresentMode(vSyncEnabled),
                false, nullptr);

        const auto [result, swapchain] = device.Get().createSwapchainKHR(createInfo);
        Assert(result == vk::Result::eSuccess);

        return SwapchainData{ swapchain, format.format, extent };
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
                    VulkanHelpers::kSubresourceRangeFlatColor);

            const auto [res, imageView] = device.createImageView(createInfo);
            Assert(res == vk::Result::eSuccess);

            imageViews.push_back(imageView);
        }

        return imageViews;
    }
}

std::unique_ptr<Swapchain> Swapchain::Create(std::shared_ptr<Device> device,
        const SwapchainInfo &swapchainInfo)
{
    const auto &[swapchain, format, extent] = SSwapchain::CreateSwapchain(GetRef(device), swapchainInfo);

    LogD << "Swapchain created" << "\n";

    return std::unique_ptr<Swapchain>(new Swapchain(device, swapchain, format, extent));
}

Swapchain::Swapchain(std::shared_ptr<Device> device_, vk::SwapchainKHR swapchain_,
        vk::Format format_, const vk::Extent2D &extent_)
    : device(device_)
    , swapchain(swapchain_)
    , format(format_)
    , extent(extent_)
{
    images = SSwapchain::RetrieveImages(device->Get(), swapchain);
    imageViews = SSwapchain::CreateImageViews(device->Get(), images, format);
}

Swapchain::~Swapchain()
{
    Destroy();
}

void Swapchain::Recreate(const SwapchainInfo &surfaceInfo)
{
    Destroy();

    const auto &[swapchain_, format_, extent_] = SSwapchain::CreateSwapchain(GetRef(device), surfaceInfo);

    swapchain = swapchain_;
    format = format_;
    extent = extent_;
    images = SSwapchain::RetrieveImages(device->Get(), swapchain);
    imageViews = SSwapchain::CreateImageViews(device->Get(), images, format);
}

void Swapchain::Destroy()
{
    for (const auto &imageView : imageViews)
    {
        device->Get().destroyImageView(imageView);
    }

    device->Get().destroySwapchainKHR(swapchain);
}
