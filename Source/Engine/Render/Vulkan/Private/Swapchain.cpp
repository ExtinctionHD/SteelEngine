#include "Engine/Render/Vulkan/Swapchain.hpp"

#include "Engine/ConsoleVariable.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"

namespace Details
{
    struct SwapchainData
    {
        vk::SwapchainKHR swapchain;
        vk::Format format;
        vk::Extent2D extent;
    };

    static bool vSyncEnabled = true;
    static CVarBool vSyncEnabledCVar("r.VSyncEnabled", vSyncEnabled);

    static int imageCount = 3;
    static CVarInt imageCountCVar("vk.SwapchainImageCount", imageCount);

    static vk::SurfaceFormatKHR SelectFormat(const std::vector<vk::SurfaceFormatKHR>& formats,
            const std::vector<vk::Format>& preferredFormats)
    {
        Assert(!formats.empty());
        Assert(!preferredFormats.empty());

        vk::SurfaceFormatKHR selectedFormat = vk::Format::eUndefined;

        for (const auto& preferredFormat : preferredFormats)
        {
            if (preferredFormat == vk::Format::eUndefined)
            {
                return formats.front();
            }

            const auto it = std::ranges::find_if(formats, [&](const vk::SurfaceFormatKHR& surfaceFormat)
                {
                    return surfaceFormat.format == preferredFormat;
                });

            if (it != formats.end())
            {
                selectedFormat = *it;
            }
        }

        Assert(selectedFormat.format != vk::Format::eUndefined);

        return selectedFormat;
    }

    static vk::Extent2D SelectExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
            const vk::Extent2D& requiredExtent)
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

    static vk::SharingMode SelectSharingMode(const Queues::Description& queuesDescription)
    {
        if (queuesDescription.graphicsFamilyIndex == queuesDescription.presentFamilyIndex)
        {
            return vk::SharingMode::eExclusive;
        }

        return vk::SharingMode::eConcurrent;
    }

    static std::vector<uint32_t> GetUniqueQueueFamilyIndices(const Queues::Description& queuesDescription)
    {
        if (queuesDescription.graphicsFamilyIndex == queuesDescription.presentFamilyIndex)
        {
            return { queuesDescription.graphicsFamilyIndex };
        }

        return { queuesDescription.graphicsFamilyIndex, queuesDescription.presentFamilyIndex };
    }

    static vk::SurfaceTransformFlagBitsKHR SelectPreTransform(vk::SurfaceCapabilitiesKHR capabilities)
    {
        if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
        {
            return vk::SurfaceTransformFlagBitsKHR::eIdentity;
        }

        return capabilities.currentTransform;
    }

    static vk::CompositeAlphaFlagBitsKHR SelectCompositeAlpha(vk::SurfaceCapabilitiesKHR capabilities)
    {
        const std::vector<vk::CompositeAlphaFlagBitsKHR> compositeAlphaFlagBits{
            vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
            vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
            vk::CompositeAlphaFlagBitsKHR::eInherit
        };

        for (const auto& compositeAlpha : compositeAlphaFlagBits)
        {
            if (capabilities.supportedCompositeAlpha & compositeAlpha)
            {
                return compositeAlpha;
            }
        }

        return vk::CompositeAlphaFlagBitsKHR::eOpaque;
    }

    static vk::PresentModeKHR SelectPresentMode(
            vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        const auto [result, supportedModes] = physicalDevice.getSurfacePresentModesKHR(surface);
        Assert(result == vk::Result::eSuccess);

        const vk::PresentModeKHR preferredMode
                = vSyncEnabled ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox;

        if (std::ranges::find(supportedModes, preferredMode) != supportedModes.end())
        {
            return preferredMode;
        }

        LogW << "Preferred present mode is not supported, falling back to immediate";

        return vk::PresentModeKHR::eImmediate;
    }

    static SwapchainData CreateSwapchain(vk::Extent2D surfaceExtent)
    {
        const Device& device = *VulkanContext::device;
        const Surface& surface = *VulkanContext::surface;

        const vk::SurfaceCapabilitiesKHR capabilities = device.GetSurfaceCapabilities(surface.Get());

        const std::vector<vk::Format> preferredFormats{ vk::Format::eUndefined };
        const vk::SurfaceFormatKHR format = SelectFormat(device.GetSurfaceFormats(surface.Get()), preferredFormats);

        const uint32_t minImageCount = std::min(capabilities.maxImageCount, static_cast<uint32_t>(Details::imageCount));

        const vk::Extent2D extent = SelectExtent(capabilities, surfaceExtent);

        const std::vector<uint32_t> uniqueQueueFamilyIndices
                = GetUniqueQueueFamilyIndices(device.GetQueuesDescription());

        const vk::PresentModeKHR presentMode = SelectPresentMode(
                device.GetPhysicalDevice(), surface.Get());

        const vk::SwapchainCreateInfoKHR createInfo({}, surface.Get(),
                minImageCount, format.format, format.colorSpace, extent, 1,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage,
                SelectSharingMode(device.GetQueuesDescription()),
                uniqueQueueFamilyIndices,
                SelectPreTransform(capabilities),
                SelectCompositeAlpha(capabilities),
                presentMode, false, nullptr);

        const auto [result, swapchain] = device.Get().createSwapchainKHR(createInfo);
        Assert(result == vk::Result::eSuccess);

        return SwapchainData{ swapchain, format.format, extent };
    }

    static std::vector<vk::Image> RetrieveImages(vk::SwapchainKHR swapchain)
    {
        const auto [result, images] = VulkanContext::device->Get().getSwapchainImagesKHR(swapchain);
        Assert(result == vk::Result::eSuccess);

        for (const auto& image : images)
        {
            VulkanContext::device->ExecuteOneTimeCommands([&image](vk::CommandBuffer commandBuffer)
                {
                    const ImageLayoutTransition layoutTransition{
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::ePresentSrcKHR,
                        PipelineBarrier::kEmpty
                    };

                    ImageHelpers::TransitImageLayout(commandBuffer, image, ImageHelpers::kFlatColor, layoutTransition);
                });
        }

        for (size_t i = 0; i < images.size(); ++i)
        {
            const std::string imageName = "Swapchain_" + std::to_string(i);

            VulkanHelpers::SetObjectName(VulkanContext::device->Get(), images[i], imageName);
        }

        return images;
    }

    static std::vector<vk::ImageView> CreateImageViews(const std::vector<vk::Image>& images, vk::Format format)
    {
        std::vector<vk::ImageView> imageViews;
        imageViews.reserve(images.size());

        const auto createImageView = [&](vk::Image image)
            {
                const vk::ImageViewCreateInfo createInfo({},
                        image, vk::ImageViewType::e2D, format,
                        ImageHelpers::kComponentMappingRGBA,
                        ImageHelpers::kFlatColor);

                const auto [result, imageView] = VulkanContext::device->Get().createImageView(createInfo);
                Assert(result == vk::Result::eSuccess);

                return imageView;
            };

        std::ranges::transform(images, std::back_inserter(imageViews), createImageView);

        return imageViews;
    }
}

std::unique_ptr<Swapchain> Swapchain::Create(vk::Extent2D surfaceExtent)
{
    const auto& [swapchain, format, extent] = Details::CreateSwapchain(surfaceExtent);

    LogD << "Swapchain created" << "\n";

    return std::unique_ptr<Swapchain>(new Swapchain(swapchain, format, extent));
}

Swapchain::Swapchain(vk::SwapchainKHR swapchain_, vk::Format format_, const vk::Extent2D& extent_)
    : swapchain(swapchain_)
    , format(format_)
    , extent(extent_)
{
    images = Details::RetrieveImages(swapchain);
    imageViews = Details::CreateImageViews(images, format);
}

Swapchain::~Swapchain()
{
    Destroy();
}

void Swapchain::Recreate(vk::Extent2D surfaceExtent)
{
    Destroy();

    const auto& [swapchain_, format_, extent_] = Details::CreateSwapchain(surfaceExtent);

    swapchain = swapchain_;
    format = format_;
    extent = extent_;
    images = Details::RetrieveImages(swapchain);
    imageViews = Details::CreateImageViews(images, format);
}

void Swapchain::Destroy() const
{
    for (const auto& imageView : imageViews)
    {
        VulkanContext::device->Get().destroyImageView(imageView);
    }

    VulkanContext::device->Get().destroySwapchainKHR(swapchain);
}
