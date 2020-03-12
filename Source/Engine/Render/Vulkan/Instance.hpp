#pragma once

class Instance
{
public:
    static std::shared_ptr<Instance> Create(std::vector<const char *> requiredExtensions);

    ~Instance();

    vk::Instance Get() const { return instance; }

private:
    vk::Instance instance;

    vk::DebugUtilsMessengerEXT debugUtilsMessenger;

    Instance(vk::Instance instance_, vk::DebugUtilsMessengerEXT debugUtilsMessenger_);
};
