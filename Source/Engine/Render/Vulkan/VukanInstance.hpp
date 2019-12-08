#pragma once

#include <vector>

#include "Engine/Render/Vulkan/Vulkan.hpp"

class VulkanInstance
{
public:
    static std::shared_ptr<VulkanInstance> Create(std::vector<const char*> requiredExtensions, bool validationEnabled);

    VulkanInstance(vk::Instance aInstance, vk::DebugUtilsMessengerEXT aDebugUtilsMessenger);
    ~VulkanInstance();

    vk::Instance Get() const { return instance; }

private:
    vk::Instance instance;

    vk::DebugUtilsMessengerEXT debugUtilsMessenger;
};
