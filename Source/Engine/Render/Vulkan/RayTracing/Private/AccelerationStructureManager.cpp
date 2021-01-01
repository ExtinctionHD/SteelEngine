#include "Engine/Render/Vulkan/RayTracing/AccelerationStructureManager.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

namespace Details
{
    vk::AccelerationStructureBuildSizesInfoKHR GetBuildSizesInfo(vk::AccelerationStructureTypeKHR type,
            const vk::AccelerationStructureGeometryKHR& geometry, uint32_t primitiveCount)
    {
        const vk::AccelerationStructureBuildGeometryInfoKHR buildInfo(
                type, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
                vk::BuildAccelerationStructureModeKHR::eBuild,
                nullptr, nullptr, 1, &geometry, nullptr, nullptr);

        return VulkanContext::device->Get().getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo,
                { primitiveCount });
    }

    vk::Buffer CreateAccelerationStructureBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage)
    {
        const vk::BufferUsageFlags bufferUsage = usage | vk::BufferUsageFlagBits::eShaderDeviceAddress;

        const BufferDescription bufferDescription{
            size, bufferUsage,
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

    vk::Buffer CreateInstanceBuffer(const std::vector<GeometryInstanceData>& instances)
    {
        std::vector<vk::AccelerationStructureInstanceKHR> vkInstances;
        vkInstances.reserve(instances.size());

        for (const auto& instance : instances)
        {
            const vk::AccelerationStructureInstanceKHR vkInstance{
                GetInstanceTransformMatrix(instance.transform),
                instance.customIndex,
                instance.mask,
                instance.sbtRecordOffset,
                instance.flags,
                VulkanContext::device->GetAddress(instance.blas)
            };

            vkInstances.push_back(vkInstance);
        }

        const vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR
                | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst;

        const BufferDescription bufferDescription{
            sizeof(vk::AccelerationStructureInstanceKHR) * instances.size(),
            usage, vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(
                bufferDescription, BufferCreateFlagBits::eStagingBuffer);

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                BufferHelpers::UpdateBuffer(commandBuffer, buffer,
                        ByteView(vkInstances), SyncScope::kAccelerationStructureBuild);
            });

        return buffer;
    }
}

vk::AccelerationStructureKHR AccelerationStructureManager::GenerateBlas(
        const GeometryVertexData& vertexData, const GeometryIndexData& indexData)
{
    const uint32_t primitiveCount = indexData.count / 3;

    const vk::AccelerationStructureTypeKHR type = vk::AccelerationStructureTypeKHR::eBottomLevel;

    const vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData(
            vertexData.format, VulkanContext::device->GetAddress(vertexData.buffer),
            vertexData.stride, vertexData.count - 1, indexData.type,
            VulkanContext::device->GetAddress(indexData.buffer), nullptr);

    const vk::AccelerationStructureGeometryDataKHR geometryData(trianglesData);

    const vk::AccelerationStructureGeometryKHR geometry(
            vk::GeometryTypeKHR::eTriangles, geometryData,
            vk::GeometryFlagBitsKHR::eOpaque);

    const vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo
            = Details::GetBuildSizesInfo(type, geometry, primitiveCount);

    const vk::Buffer storageBuffer = Details::CreateAccelerationStructureBuffer(
            buildSizesInfo.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR);

    const vk::Buffer buildScratchBuffer = Details::CreateAccelerationStructureBuffer(
            buildSizesInfo.buildScratchSize, vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);

    const vk::AccelerationStructureCreateInfoKHR createInfo({}, storageBuffer, 0,
            buildSizesInfo.accelerationStructureSize, type, vk::DeviceAddress());

    const auto [result, blas] = VulkanContext::device->Get().createAccelerationStructureKHR(createInfo);
    Assert(result == vk::Result::eSuccess);

    const vk::AccelerationStructureBuildGeometryInfoKHR buildInfo(
            type, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            vk::BuildAccelerationStructureModeKHR::eBuild,
            nullptr, blas, 1, &geometry, nullptr,
            VulkanContext::device->GetAddress(buildScratchBuffer));

    const vk::AccelerationStructureBuildRangeInfoKHR offsetInfo(primitiveCount, 0, 0, 0);
    const vk::AccelerationStructureBuildRangeInfoKHR* pOffsetInfo = &offsetInfo;

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.buildAccelerationStructuresKHR({ buildInfo }, { pOffsetInfo });
        });

    VulkanContext::bufferManager->DestroyBuffer(buildScratchBuffer);

    accelerationStructures.emplace(blas, storageBuffer);

    return blas;
}

vk::AccelerationStructureKHR AccelerationStructureManager::GenerateTlas(
        const std::vector<GeometryInstanceData>& instances)
{
    const uint32_t instanceCount = static_cast<uint32_t>(instances.size());

    const vk::AccelerationStructureTypeKHR type = vk::AccelerationStructureTypeKHR::eTopLevel;

    const vk::Buffer instanceBuffer = Details::CreateInstanceBuffer(instances);

    const vk::AccelerationStructureGeometryInstancesDataKHR instancesData(
            false, VulkanContext::device->GetAddress(instanceBuffer));

    const vk::AccelerationStructureGeometryDataKHR geometryData(instancesData);

    const vk::AccelerationStructureGeometryKHR geometry(
            vk::GeometryTypeKHR::eInstances, geometryData,
            vk::GeometryFlagBitsKHR::eOpaque);

    const vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo
            = Details::GetBuildSizesInfo(type, geometry, instanceCount);

    const vk::Buffer storageBuffer = Details::CreateAccelerationStructureBuffer(
            buildSizesInfo.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR);

    const vk::Buffer buildScratchBuffer = Details::CreateAccelerationStructureBuffer(
            buildSizesInfo.buildScratchSize, vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);

    const vk::AccelerationStructureCreateInfoKHR createInfo({}, storageBuffer, 0,
            buildSizesInfo.accelerationStructureSize, type, vk::DeviceAddress());

    const auto [result, blas] = VulkanContext::device->Get().createAccelerationStructureKHR(createInfo);
    Assert(result == vk::Result::eSuccess);

    const vk::AccelerationStructureBuildGeometryInfoKHR buildInfo(
            type, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            vk::BuildAccelerationStructureModeKHR::eBuild,
            nullptr, blas, 1, &geometry, nullptr,
            VulkanContext::device->GetAddress(buildScratchBuffer));

    const vk::AccelerationStructureBuildRangeInfoKHR offsetInfo(instanceCount, 0, 0, 0);
    const vk::AccelerationStructureBuildRangeInfoKHR* pOffsetInfo = &offsetInfo;

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.buildAccelerationStructuresKHR({ buildInfo }, { pOffsetInfo });
        });

    VulkanContext::bufferManager->DestroyBuffer(buildScratchBuffer);

    accelerationStructures.emplace(blas, storageBuffer);

    return blas;
}

void AccelerationStructureManager::DestroyAccelerationStructure(vk::AccelerationStructureKHR accelerationStructure)
{
    if (const auto it = tlasInstanceBuffers.find(accelerationStructure); it != tlasInstanceBuffers.end())
    {
        VulkanContext::bufferManager->DestroyBuffer(it->second);
        tlasInstanceBuffers.erase(it);
    }

    const auto it = accelerationStructures.find(accelerationStructure);
    Assert(it != accelerationStructures.end());

    VulkanContext::device->Get().destroyAccelerationStructureKHR(accelerationStructure);
    VulkanContext::bufferManager->DestroyBuffer(it->second);

    accelerationStructures.erase(it);
}
