#pragma once

enum class eValidation
{
    kEnabled,
    kDisabled
};

class Instance
{
public:
    static std::shared_ptr<Instance> Create(std::vector<const char*> requiredExtensions, eValidation validation);

    ~Instance();

    vk::Instance Get() const { return instance; }

private:
    vk::Instance instance;

    vk::DebugUtilsMessengerEXT debugUtilsMessenger;

    Instance(vk::Instance aInstance, vk::DebugUtilsMessengerEXT aDebugUtilsMessenger);
};
