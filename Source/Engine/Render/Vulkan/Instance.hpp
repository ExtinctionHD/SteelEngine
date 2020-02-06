#pragma once

#include <vector>

class Instance
{
public:
    enum class eValidation
    {
        kEnabled,
        kDisabled
    };

    static std::shared_ptr<Instance> Create(std::vector<const char*> requiredExtensions, eValidation validation);
    ~Instance();

    vk::Instance Get() const { return instance; }

private:
    vk::Instance instance;

    vk::DebugUtilsMessengerEXT debugUtilsMessenger;

    Instance(vk::Instance aInstance, vk::DebugUtilsMessengerEXT aDebugUtilsMessenger);
};
