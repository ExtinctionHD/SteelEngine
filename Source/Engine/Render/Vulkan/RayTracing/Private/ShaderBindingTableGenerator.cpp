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

    Bytes shaderGroupsData(handleSize * groupCount);

    const vk::Result result = device->Get().getRayTracingShaderGroupHandlesNV<uint8_t>(
            pipeline.Get(), 0, groupCount, shaderGroupsData);
    Assert(result == vk::Result::eSuccess);

    const BufferDescription description{
        shaderGroupsData.size(),
        vk::BufferUsageFlagBits::eRayTracingNV | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    const BufferHandle shaderBindingTable = bufferManager->CreateBuffer(description, shaderGroupsData);
    shaderBindingTable->FreeCpuMemory();

    return shaderBindingTable;
}
