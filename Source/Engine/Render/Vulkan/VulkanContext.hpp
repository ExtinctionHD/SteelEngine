#pragma once

#include <memory>

class VulkanInstance;
class VulkanDevice;
class VulkanSurface;

struct GLFWwindow;

class VulkanContext
{
public:
    static void Initialize();
    static VulkanContext *Get();

    void CreateSurface(GLFWwindow *window);

private:
    inline static VulkanContext *vulkanContext = nullptr;

    VulkanContext();

    std::unique_ptr<VulkanInstance> vulkanInstance;
    std::unique_ptr<VulkanDevice> vulkanDevice;
    std::unique_ptr<VulkanSurface> vulkanSurface;
};
