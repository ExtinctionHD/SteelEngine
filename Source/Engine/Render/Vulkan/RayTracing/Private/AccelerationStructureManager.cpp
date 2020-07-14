#include "Engine/Render/Vulkan/RayTracing/AccelerationStructureManager.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

namespace SAccelerationStructureManager
{
    vk::Buffer CreateScratchBuffer(vk::AccelerationStructureKHR accelerationStructure)
    {
        const vk::Device device = VulkanContext::device->Get();

        const vk::AccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo(
                vk::AccelerationStructureMemoryRequirementsTypeKHR::eBuildScratch,
                vk::AccelerationStructureBuildTypeKHR::eDevice,
                accelerationStructure);

        const vk::MemoryRequirements2 memoryRequirements
                = device.getAccelerationStructureMemoryRequirementsKHR(memoryRequirementsInfo);

        const BufferDescription bufferDescription{
            memoryRequirements.memoryRequirements.size,
            vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(
                bufferDescription, BufferCreateFlags::kNone);

        return buffer;
    }

    vk::TransformMatrixKHR GetInstanceTransformMatrix(const glm::mat4 transform)
    {
        const glm::mat4 transposedTransform = glm::transpose(transform);

        std::array<std::array<float, 4>, 3> transposedData;

        std::memcpy(&transposedData, &transposedTransform, sizeof(vk::TransformMatrixKHR));

        return vk::TransformMatrixKHR(transposedData);
    }

    vk::Buffer CreateInstanceBuffer(const std::vector<GeometryInstanceData> &instances)
    {
        const uint32_t instanceCount = static_cast<uint32_t>(instances.size());

        const vk::GeometryInstanceFlagsKHR instanceFlags = vk::GeometryInstanceFlagBitsKHR::eForceOpaque
                | vk::GeometryInstanceFlagBitsKHR::eTriangleFrontCounterclockwise;

        std::vector<vk::AccelerationStructureInstanceKHR> vkInstances(instanceCount);

        for (uint32_t i = 0; i < instanceCount; ++i)
        {
            const GeometryInstanceData &instance = instances[i];

            vk::AccelerationStructureInstanceKHR &vkInstance = vkInstances[i];
            vkInstance.setTransform(GetInstanceTransformMatrix(instance.transform));
            vkInstance.setInstanceCustomIndex(i);
            vkInstance.setMask(0xFF);
            vkInstance.setFlags(instanceFlags);
            vkInstance.setInstanceShaderBindingTableRecordOffset(0);
            vkInstance.setAccelerationStructureReference(VulkanContext::device->GetAddress(instance.blas));
        }

        const vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eRayTracingKHR
                | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst;

        const BufferDescription description{
            sizeof(vk::AccelerationStructureInstanceKHR) * instanceCount,
            usage, vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(
                description, BufferCreateFlagBits::eStagingBuffer);

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                VulkanContext::bufferManager->UpdateBuffer(commandBuffer, buffer, GetByteView(vkInstances));

                const PipelineBarrier barrier{
                    SyncScope::kTransferWrite,
                    SyncScope::kAccelerationStructureBuild
                };

                BufferHelpers::InsertPipelineBarrier(commandBuffer, buffer, barrier);
            });

        return buffer;
    }

    void BuildAccelerationStructure(vk::AccelerationStructureKHR accelerationStructure,
            const vk::AccelerationStructureGeometryKHR &geometry, uint32_t primitiveCount)
    {
        const vk::AccelerationStructureTypeKHR type = geometry.geometryType == vk::GeometryTypeKHR::eInstances
                ? vk::AccelerationStructureTypeKHR::eTopLevel : vk::AccelerationStructureTypeKHR::eBottomLevel;

        const vk::AccelerationStructureGeometryKHR *pGeometry = &geometry;

        const vk::Buffer scratchBuffer = SAccelerationStructureManager::CreateScratchBuffer(accelerationStructure);

        const vk::AccelerationStructureBuildGeometryInfoKHR buildInfo(
                type, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
                false, nullptr, accelerationStructure, false, 1, &pGeometry,
                VulkanContext::device->GetAddress(scratchBuffer));

        const vk::AccelerationStructureBuildOffsetInfoKHR offsetInfo(primitiveCount, 0, 0, 0);
        const vk::AccelerationStructureBuildOffsetInfoKHR *pOffsetInfo = &offsetInfo;

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                commandBuffer.buildAccelerationStructureKHR({ buildInfo }, { pOffsetInfo });
            });

        VulkanContext::bufferManager->DestroyBuffer(scratchBuffer);
    }
}

vk::AccelerationStructureKHR AccelerationStructureManager::GenerateBlas(
        const GeometryVertexData &vertexData, const GeometryIndexData &indexData)
{
    const vk::AccelerationStructureCreateGeometryTypeInfoKHR geometryInfo(
            vk::GeometryTypeKHR::eTriangles, indexData.count / 3, indexData.type,
            vertexData.count, vertexData.format, true);

    const vk::AccelerationStructureCreateInfoKHR createInfo(0,
            vk::AccelerationStructureTypeKHR::eBottomLevel,
            vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            1, &geometryInfo, vk::DeviceAddress());

    const vk::AccelerationStructureKHR blas = VulkanContext::memoryManager->CreateAccelerationStructure(createInfo);

    const vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData(
            vertexData.format, VulkanContext::device->GetAddress(vertexData.buffer), vertexData.stride,
            indexData.type, VulkanContext::device->GetAddress(indexData.buffer), nullptr);

    const vk::AccelerationStructureGeometryDataKHR geometryData(trianglesData);

    const vk::AccelerationStructureGeometryKHR geometry(
            vk::GeometryTypeKHR::eTriangles, geometryData,
            vk::GeometryFlagBitsKHR::eOpaque);

    SAccelerationStructureManager::BuildAccelerationStructure(blas, geometry, indexData.count / 3);

    accelerationStructures.push_back(blas);

    return blas;
}

vk::AccelerationStructureKHR AccelerationStructureManager::GenerateTlas(
        const std::vector<GeometryInstanceData> &instances)
{
    const uint32_t instanceCount = static_cast<uint32_t>(instances.size());

    const vk::AccelerationStructureCreateGeometryTypeInfoKHR geometryInfo(
            vk::GeometryTypeKHR::eInstances, instanceCount,
            vk::IndexType(), 0, vk::Format(), false);

    const vk::AccelerationStructureCreateInfoKHR createInfo(0,
            vk::AccelerationStructureTypeKHR::eTopLevel,
            vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            1, &geometryInfo, vk::DeviceAddress());

    const vk::AccelerationStructureKHR tlas = VulkanContext::memoryManager->CreateAccelerationStructure(createInfo);

    const vk::Buffer instanceBuffer = SAccelerationStructureManager::CreateInstanceBuffer(instances);

    const vk::AccelerationStructureGeometryInstancesDataKHR instancesData(
            false, VulkanContext::device->GetAddress(instanceBuffer));

    const vk::AccelerationStructureGeometryDataKHR geometryData(instancesData);

    const vk::AccelerationStructureGeometryKHR geometry(
            vk::GeometryTypeKHR::eInstances, geometryData,
            vk::GeometryFlagBitsKHR::eOpaque);

    SAccelerationStructureManager::BuildAccelerationStructure(tlas, geometry, instanceCount);

    accelerationStructures.push_back(tlas);
    tlasInstanceBuffers.emplace(tlas, instanceBuffer);

    return tlas;
}

void AccelerationStructureManager::DestroyAccelerationStructure(vk::AccelerationStructureKHR accelerationStructure)
{
    const auto tlasIt = tlasInstanceBuffers.find(accelerationStructure);

    if (tlasIt != tlasInstanceBuffers.end())
    {
        VulkanContext::bufferManager->DestroyBuffer(tlasIt->second);
        tlasInstanceBuffers.erase(tlasIt);
    }

    const auto it = std::find(accelerationStructures.begin(), accelerationStructures.end(), accelerationStructure);
    Assert(it != accelerationStructures.end());

    VulkanContext::memoryManager->DestroyAccelerationStructure(accelerationStructure);

    accelerationStructures.erase(it);
}
