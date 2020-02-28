#include "Engine/Render/Vulkan/RayTracing/ShaderBindingTableGenerator.hpp"

#include "Engine/Render/Vulkan/RayTracing/RayTracingPipeline.hpp"

ShaderBindingTableGenerator::ShaderBindingTableGenerator(std::shared_ptr<Device> aDevice,
        std::shared_ptr<BufferManager> aBufferManager)
    : device(aDevice)
    , bufferManager(aBufferManager)
{
    handleSize = device->GetRayTracingProperties().shaderGroupHandleSize;
}

BufferHandle ShaderBindingTableGenerator::GenerateShaderBindingTable(const RayTracingPipeline &pipeline) const
{
    const uint32_t groupCount = pipeline.GetShaderGroupCount();

    Bytes shaderGroups(handleSize * groupCount);

    device->Get().getRayTracingShaderGroupHandlesNV(pipeline.Get(),
            0, groupCount, shaderGroups.size(), shaderGroups.data());

    const BufferDescription description{
        shaderGroups.size(),
        vk::BufferUsageFlagBits::eRayTracingNV | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    const BufferHandle shaderBindingTable = bufferManager->CreateBuffer(description, shaderGroups);
    shaderBindingTable->FreeCpuMemory();

    return shaderBindingTable;
}
