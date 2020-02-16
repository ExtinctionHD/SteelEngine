#pragma once

class Instance
{
public:
    static std::shared_ptr<Instance> Create(std::vector<const char*> requiredExtensions, bool validationEnabled);

    ~Instance();

    vk::Instance Get() const { return instance; }

private:
    vk::Instance instance;

    vk::DebugUtilsMessengerEXT debugUtilsMessenger;

    Instance(vk::Instance aInstance, vk::DebugUtilsMessengerEXT aDebugUtilsMessenger);
};
