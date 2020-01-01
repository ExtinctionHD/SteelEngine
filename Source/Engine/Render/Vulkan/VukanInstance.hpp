#pragma once

#include <vector>

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
