#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"

class RayTracingPipeline;

class ShaderBindingTableGenerator
{
public:
    ShaderBindingTableGenerator(std::shared_ptr<Device> aDevice, std::shared_ptr<BufferManager> aBufferManager);

    BufferHandle GenerateShaderBindingTable(const RayTracingPipeline &pipeline) const;

private:
    std::shared_ptr<Device> device;
    std::shared_ptr<BufferManager> bufferManager;

    uint32_t handleSize;
};
