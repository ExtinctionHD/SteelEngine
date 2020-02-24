#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"

std::unique_ptr<RayTracingPipeline> RayTracingPipeline::Create(std::shared_ptr<Device> device)
{
    return std::unique_ptr<RayTracingPipeline>(new RayTracingPipeline(device));
}

RayTracingPipeline::RayTracingPipeline(std::shared_ptr<Device> aDevice)
    : device(aDevice)
{}

RayTracingPipeline::~RayTracingPipeline()
{
    device->Get().destroyPipeline(pipeline);
}
