#pragma once

#include "Engine/Render/Vulkan/Device.hpp"

class RayTracingPipeline
{
public:
    static std::unique_ptr<RayTracingPipeline> Create(std::shared_ptr<Device> device);

    ~RayTracingPipeline();

    vk::Pipeline Get() const { return pipeline; }

private:
    RayTracingPipeline(std::shared_ptr<Device> aDevice);

    std::shared_ptr<Device> device;

    vk::Pipeline pipeline;
};
