#pragma once

#include <vector>

#include "Engine/Render/Vulkan/Vulkan.hpp"

class VulkanInstance
{
public:
    enum class eValidation
    {
        kEnabled,
        kDisabled
    };

    static std::shared_ptr<VulkanInstance> Create(std::vector<const char*> requiredExtensions, eValidation validation);

    VulkanInstance(vk::Instance aInstance, vk::DebugUtilsMessengerEXT aDebugUtilsMessenger);
    ~VulkanInstance();

    vk::Instance Get() const { return instance; }

private:
    vk::Instance instance;

    vk::DebugUtilsMessengerEXT debugUtilsMessenger;
};
