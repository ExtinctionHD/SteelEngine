#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Logger.hpp"
#include "Utils/Assert.hpp"

namespace Details
{
    bool RequiredDeviceExtensionsSupported(vk::PhysicalDevice physicalDevice,
            const std::vector<const char*> &requiredDeviceExtensions)
    {
        const auto [result, deviceExtensions] = physicalDevice.enumerateDeviceExtensionProperties();

        for (const auto &requiredDeviceExtension : requiredDeviceExtensions)
        {
            const auto pred = [&requiredDeviceExtension](const auto &extension)
                {
                    return std::strcmp(extension.extensionName, requiredDeviceExtension) == 0;
                };

            const auto it = std::find_if(deviceExtensions.begin(), deviceExtensions.end(), pred);

            if (it == deviceExtensions.end())
            {
                LogE << "Required device extension not found: " << requiredDeviceExtension << "\n";
                return false;
            }
        }

        return true;
    }

    bool IsSuitablePhysicalDevice(vk::PhysicalDevice physicalDevice,
            const std::vector<const char*> &requiredDeviceExtensions)
    {
        return RequiredDeviceExtensionsSupported(physicalDevice, requiredDeviceExtensions);
    }

    vk::PhysicalDevice FindSuitablePhysicalDevice(vk::Instance instance,
            const std::vector<const char*> &requiredDeviceExtensions)
    {
        const auto [result, physicalDevices] = instance.enumeratePhysicalDevices();
        Assert(result == vk::Result::eSuccess);

        const auto pred = [&requiredDeviceExtensions](const auto &physicalDevice)
            {
                return Details::IsSuitablePhysicalDevice(physicalDevice, requiredDeviceExtensions);
            };

        const auto it = std::find_if(physicalDevices.begin(), physicalDevices.end(), pred);
        Assert(it != physicalDevices.end());

        return *it;
    }

    uint32_t FindGraphicsQueueFamilyIndex(vk::PhysicalDevice physicalDevice)
    {
        const auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        const auto pred = [](const vk::QueueFamilyProperties &queueFamily)
            {
                return queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics;
            };

        const auto it = std::find_if(queueFamilies.begin(), queueFamilies.end(), pred);

        Assert(it != queueFamilies.end());

        return static_cast<uint32_t>(std::distance(queueFamilies.begin(), it));
    }

    std::optional<uint32_t> FindCommonQueueFamilyIndex(
            vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        const auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        for (uint32_t i = 0; i < queueFamilies.size(); ++i)
        {
            const auto [result, supportSurface] = physicalDevice.getSurfaceSupportKHR(i, surface);
            Assert(result == vk::Result::eSuccess);

            const bool isSuitableQueueFamily = queueFamilies[i].queueCount > 0
                    && queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics && supportSurface;

            if (isSuitableQueueFamily)
            {
                return i;
            }
        }

        return std::nullopt;
    }

    std::optional<uint32_t> FindPresentQueueFamilyIndex(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        const auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        for (uint32_t i = 0; i < queueFamilies.size(); ++i)
        {
            const auto [result, supportSurface] = physicalDevice.getSurfaceSupportKHR(i, surface);
            Assert(result == vk::Result::eSuccess);

            if (queueFamilies[i].queueCount > 0 && supportSurface)
            {
                return i;
            }
        }

        return std::nullopt;
    }

    Queues::Description GetQueuesDescription(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        const uint32_t graphicsQueueFamilyIndex = FindGraphicsQueueFamilyIndex(physicalDevice);

        const auto [result, supportSurface] = physicalDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, surface);
        Assert(result == vk::Result::eSuccess);

        if (supportSurface)
        {
            return Queues::Description{ graphicsQueueFamilyIndex, graphicsQueueFamilyIndex };
        }

        const std::optional<uint32_t> commonQueueFamilyIndex
                = FindCommonQueueFamilyIndex(physicalDevice, surface);

        if (commonQueueFamilyIndex.has_value())
        {
            return Queues::Description{ graphicsQueueFamilyIndex, graphicsQueueFamilyIndex };
        }

        const std::optional<uint32_t> presentQueueFamilyIndex = FindPresentQueueFamilyIndex(physicalDevice, surface);
        Assert(presentQueueFamilyIndex.has_value());

        return Queues::Description{ graphicsQueueFamilyIndex, presentQueueFamilyIndex.value() };
    }

    std::vector<vk::DeviceQueueCreateInfo> CreateQueuesCreateInfo(
            const Queues::Description &queuesDescription)
    {
        static const float queuePriority = 0.0;

        std::vector<vk::DeviceQueueCreateInfo> queuesCreateInfo{
            vk::DeviceQueueCreateInfo({}, queuesDescription.graphicsFamilyIndex, 1, &queuePriority)
        };

        if (queuesDescription.graphicsFamilyIndex != queuesDescription.presentFamilyIndex)
        {
            queuesCreateInfo.emplace_back(vk::DeviceQueueCreateFlags(),
                    queuesDescription.presentFamilyIndex, 1, &queuePriority);
        }

        return queuesCreateInfo;
    }

    vk::PhysicalDeviceFeatures2 GetPhysicalDeviceFeatures(const Device::Features &deviceFeatures)
    {
        vk::PhysicalDeviceFeatures features;
        features.setSamplerAnisotropy(deviceFeatures.samplerAnisotropy);

        vk::PhysicalDeviceRayTracingFeaturesKHR rayTracingFeatures;
        rayTracingFeatures.setRayTracing(deviceFeatures.rayTracing);

        vk::PhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures;
        descriptorIndexingFeatures.setRuntimeDescriptorArray(deviceFeatures.descriptorIndexing);
        descriptorIndexingFeatures.setShaderStorageBufferArrayNonUniformIndexing(deviceFeatures.descriptorIndexing);
        descriptorIndexingFeatures.setShaderUniformBufferArrayNonUniformIndexing(deviceFeatures.descriptorIndexing);
        descriptorIndexingFeatures.setShaderSampledImageArrayNonUniformIndexing(deviceFeatures.descriptorIndexing);

        vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures;
        bufferDeviceAddressFeatures.setBufferDeviceAddress(deviceFeatures.bufferDeviceAddress);

        using FeaturesStructureChain = vk::StructureChain<vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceRayTracingFeaturesKHR, vk::PhysicalDeviceDescriptorIndexingFeatures,
            vk::PhysicalDeviceBufferDeviceAddressFeatures>;

        static FeaturesStructureChain featuresStructureChain(vk::PhysicalDeviceFeatures2(features),
                rayTracingFeatures, descriptorIndexingFeatures, bufferDeviceAddressFeatures);

        return featuresStructureChain.get<vk::PhysicalDeviceFeatures2>();
    }

    vk::PhysicalDeviceRayTracingPropertiesKHR GetRayTracingProperties(vk::PhysicalDevice physicalDevice)
    {
        return physicalDevice.getProperties2<
            vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPropertiesKHR>().get<
            vk::PhysicalDeviceRayTracingPropertiesKHR>();
    }

    vk::CommandPool CreateCommandPool(vk::Device device, vk::CommandPoolCreateFlags flags, uint32_t queueFamilyIndex)
    {
        const vk::CommandPoolCreateInfo createInfo(flags, queueFamilyIndex);

        const auto [result, commandPool] = device.createCommandPool(createInfo);
        Assert(result == vk::Result::eSuccess);

        return commandPool;
    }
}

std::unique_ptr<Device> Device::Create(const Features &requiredFeatures,
        const std::vector<const char*> &requiredExtensions)
{
    const auto physicalDevice = Details::FindSuitablePhysicalDevice(
            VulkanContext::instance->Get(), requiredExtensions);

    const Queues::Description queuesDescription = Details::GetQueuesDescription(physicalDevice,
            VulkanContext::surface->Get());

    const std::vector<vk::DeviceQueueCreateInfo> queueCreatesInfo
            = Details::CreateQueuesCreateInfo(queuesDescription);

    const vk::DeviceCreateInfo createInfo({},
            static_cast<uint32_t>(queueCreatesInfo.size()), queueCreatesInfo.data(), 0, nullptr,
            static_cast<uint32_t>(requiredExtensions.size()), requiredExtensions.data(), nullptr);

    vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceFeatures2> structures(createInfo,
            Details::GetPhysicalDeviceFeatures(requiredFeatures));

    const auto [result, device] = physicalDevice.createDevice(structures.get<vk::DeviceCreateInfo>());
    Assert(result == vk::Result::eSuccess);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

    const vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
    LogI << "GPU selected: " << properties.deviceName << "\n";

    LogD << "Device created" << "\n";

    return std::unique_ptr<Device>(new Device(device, physicalDevice, queuesDescription));
}

Device::Device(vk::Device device_, vk::PhysicalDevice physicalDevice_, const Queues::Description &queuesDescription_)
    : device(device_)
    , physicalDevice(physicalDevice_)
    , queuesDescription(queuesDescription_)
{
    properties = physicalDevice.getProperties();
    rayTracingProperties = Details::GetRayTracingProperties(physicalDevice);

    queues.graphics = device.getQueue(queuesDescription.graphicsFamilyIndex, 0);
    queues.present = device.getQueue(queuesDescription.presentFamilyIndex, 0);

    commandPools[CommandBufferType::eOneTime] = Details::CreateCommandPool(device,
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer
            | vk::CommandPoolCreateFlagBits::eTransient,
            queuesDescription.graphicsFamilyIndex);

    commandPools[CommandBufferType::eLongLived] = Details::CreateCommandPool(device,
            vk::CommandPoolCreateFlags(), queuesDescription.graphicsFamilyIndex);

    oneTimeCommandsSync.fence = VulkanHelpers::CreateFence(device, vk::FenceCreateFlags());
}

Device::~Device()
{
    VulkanHelpers::DestroyCommandBufferSync(device, oneTimeCommandsSync);
    for (const auto &[type, commandPool] : commandPools)
    {
        device.destroyCommandPool(commandPool);
    }
    device.destroy();
}

vk::SurfaceCapabilitiesKHR Device::GetSurfaceCapabilities(vk::SurfaceKHR surface) const
{
    const auto [result, capabilities] = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    Assert(result == vk::Result::eSuccess);

    return capabilities;
}

std::vector<vk::SurfaceFormatKHR> Device::GetSurfaceFormats(vk::SurfaceKHR surface) const
{
    const auto [result, formats] = physicalDevice.getSurfaceFormatsKHR(surface);
    Assert(result == vk::Result::eSuccess);

    return formats;
}

uint32_t Device::GetMemoryTypeIndex(uint32_t typeBits, vk::MemoryPropertyFlags requiredProperties) const
{
    const vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

    std::optional<uint32_t> index;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        const vk::MemoryPropertyFlags currentTypeProperties = memoryProperties.memoryTypes[i].propertyFlags;
        const bool meetsRequirements = (currentTypeProperties & requiredProperties) == requiredProperties;
        const bool isSuitableType = typeBits & (1 << i);

        if (isSuitableType && meetsRequirements)
        {
            index = i;
            break;
        }
    }

    Assert(index.has_value());

    return index.value();
}

vk::DeviceAddress Device::GetAddress(vk::Buffer buffer) const
{
    return device.getBufferAddress({ buffer });
}

vk::DeviceAddress Device::GetAddress(vk::AccelerationStructureKHR accelerationStructure) const
{
    return device.getAccelerationStructureAddressKHR({ accelerationStructure });
}

void Device::ExecuteOneTimeCommands(DeviceCommands commands) const
{
    vk::CommandBuffer commandBuffer;

    const vk::CommandPool commandPool = commandPools.at(CommandBufferType::eOneTime);
    const vk::CommandBufferAllocateInfo allocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);

    vk::Result result = device.allocateCommandBuffers(&allocateInfo, &commandBuffer);
    Assert(result == vk::Result::eSuccess);

    VulkanHelpers::SubmitCommandBuffer(queues.graphics, commandBuffer, commands, oneTimeCommandsSync);

    VulkanHelpers::WaitForFences(device, { oneTimeCommandsSync.fence });

    result = commandBuffer.reset(vk::CommandBufferResetFlags());
    Assert(result == vk::Result::eSuccess);

    result = device.resetFences({ oneTimeCommandsSync.fence });
    Assert(result == vk::Result::eSuccess);
}

vk::CommandBuffer Device::AllocateCommandBuffer(CommandBufferType type) const
{
    vk::CommandBuffer commandBuffer;

    const vk::CommandBufferAllocateInfo allocateInfo(commandPools.at(type), vk::CommandBufferLevel::ePrimary, 1);

    const vk::Result result = device.allocateCommandBuffers(&allocateInfo, &commandBuffer);
    Assert(result == vk::Result::eSuccess);

    return commandBuffer;
}

void Device::WaitIdle() const
{
    const vk::Result result = device.waitIdle();
    Assert(result == vk::Result::eSuccess);
}
