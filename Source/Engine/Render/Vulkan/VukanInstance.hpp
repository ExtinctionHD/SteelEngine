#pragma once

#include <vector>

#include "Engine/Render/Vulkan/Vulkan.hpp"

class VulkanInstance
{
public:
    static std::unique_ptr<VulkanInstance> Create(std::vector<const char*> requiredExtensions, bool validationEnabled);

    VulkanInstance(vk::Instance aInstance, vk::DebugUtilsMessengerEXT aDebugUtilsMessenger);

    vk::Instance Get() const { return instance.get(); }

private:
    vk::UniqueInstance instance;
    vk::UniqueDebugUtilsMessengerEXT debugUtilsMessenger;
};
