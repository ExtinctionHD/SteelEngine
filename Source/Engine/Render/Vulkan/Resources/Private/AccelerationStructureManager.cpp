#include "Engine/Render/Vulkan/Resources/AccelerationStructureManager.hpp"

#include "Engine/Render/Vulkan/Resources/ResourceHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace Details
{
    constexpr vk::BuildAccelerationStructureFlagsKHR kBlasBuildFlags
            = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;

    constexpr vk::BuildAccelerationStructureFlagsKHR kTlasBuildFlags
            = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;

    static vk::BuildAccelerationStructureFlagsKHR GetBuildFlags(vk::AccelerationStructureTypeKHR type)
    {
        return type == vk::AccelerationStructureTypeKHR::eTopLevel ? kTlasBuildFlags : kBlasBuildFlags;
    }

    static vk::AccelerationStructureBuildSizesInfoKHR GetBuildSizesInfo(vk::AccelerationStructureTypeKHR type,
            const vk::AccelerationStructureGeometryKHR& geometry, uint32_t primitiveCount)
    {
        const vk::AccelerationStructureBuildGeometryInfoKHR buildInfo(
                type, GetBuildFlags(type),
                vk::BuildAccelerationStructureModeKHR::eBuild,
                nullptr, nullptr, 1, &geometry, nullptr,
                vk::DeviceOrHostAddressKHR(), nullptr);

        return VulkanContext::device->Get().getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo,
                { primitiveCount });
    }

    static vk::Buffer CreateAccelerationStructureBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage)
    {
        const BufferDescription bufferDescription{
            size, usage | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        BufferCreateFlags createFlags;

        if (usage & vk::BufferUsageFlagBits::eStorageBuffer)
        {
            createFlags |= BufferCreateFlagBits::eScratchBuffer;
        }

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(bufferDescription, createFlags);

        return buffer;
    }

    static vk::Buffer CreateEmptyInstanceBuffer(const TlasInstances& instances)
    {
        const vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eShaderDeviceAddress
                | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;

        const size_t size = instances.size() * sizeof(vk::AccelerationStructureInstanceKHR);

        const vk::Buffer buffer = BufferHelpers::CreateEmptyBuffer(usage, size);

        return buffer;
    }
}

vk::AccelerationStructureKHR AccelerationStructureManager::GenerateBlas(const BlasGeometryData& geometryData)
{
    constexpr vk::AccelerationStructureTypeKHR type = vk::AccelerationStructureTypeKHR::eBottomLevel;

    constexpr vk::BufferUsageFlags bufferUsage = vk::BufferUsageFlagBits::eShaderDeviceAddress
            | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;

    const vk::Buffer vertexBuffer = BufferHelpers::CreateBufferWithData(bufferUsage, ByteView(geometryData.vertices));
    const vk::Buffer indexBuffer = BufferHelpers::CreateBufferWithData(bufferUsage, ByteView(geometryData.indices));

    const vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData(
            geometryData.vertexFormat, VulkanContext::device->GetAddress(vertexBuffer),
            geometryData.vertexStride, geometryData.vertexCount - 1,
            geometryData.indexType, VulkanContext::device->GetAddress(indexBuffer), nullptr);

    const vk::AccelerationStructureGeometryKHR geometry(
            vk::GeometryTypeKHR::eTriangles, trianglesData,
            vk::GeometryFlagsKHR());

    const uint32_t primitiveCount = geometryData.indexCount / 3;

    const vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo
            = Details::GetBuildSizesInfo(type, geometry, primitiveCount);

    AccelerationStructureBuffers buffers;

    buffers.scratchBuffer = Details::CreateAccelerationStructureBuffer(
            buildSizesInfo.buildScratchSize, vk::BufferUsageFlagBits::eStorageBuffer);

    buffers.storageBuffer = Details::CreateAccelerationStructureBuffer(
            buildSizesInfo.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR);

    const vk::AccelerationStructureCreateInfoKHR createInfo({}, buffers.storageBuffer, 0,
            buildSizesInfo.accelerationStructureSize, type, vk::DeviceAddress());

    const auto [result, blas] = VulkanContext::device->Get().createAccelerationStructureKHR(createInfo);

    const vk::AccelerationStructureBuildGeometryInfoKHR buildInfo(
            type, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            vk::BuildAccelerationStructureModeKHR::eBuild,
            nullptr, blas, 1, &geometry, nullptr,
            VulkanContext::device->GetAddress(buffers.scratchBuffer));

    const vk::AccelerationStructureBuildRangeInfoKHR rangeInfo(primitiveCount, 0, 0, 0);
    const vk::AccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.buildAccelerationStructuresKHR({ buildInfo }, { pRangeInfo });
        });

    ResourceHelpers::DestroyResource(vertexBuffer);
    ResourceHelpers::DestroyResource(indexBuffer);

    accelerationStructures.emplace(blas, buffers);

    return blas;
}

vk::AccelerationStructureKHR AccelerationStructureManager::GenerateTlas(const TlasInstances& instances)
{
    constexpr vk::AccelerationStructureTypeKHR type = vk::AccelerationStructureTypeKHR::eTopLevel;

    AccelerationStructureBuffers buffers;

    buffers.sourceBuffer = Details::CreateEmptyInstanceBuffer(instances);

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            BufferHelpers::UpdateBuffer(commandBuffer, buffers.sourceBuffer, GetByteView(instances),
                    SyncScope::kWaitForNone, SyncScope::kAccelerationStructureWrite);
        });

    const vk::AccelerationStructureGeometryInstancesDataKHR instancesData(
            false, VulkanContext::device->GetAddress(buffers.sourceBuffer));

    const vk::AccelerationStructureGeometryDataKHR geometryData(instancesData);

    const vk::AccelerationStructureGeometryKHR geometry(
            vk::GeometryTypeKHR::eInstances, geometryData,
            vk::GeometryFlagBitsKHR::eOpaque);

    const uint32_t instanceCount = static_cast<uint32_t>(instances.size());

    const vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo
            = Details::GetBuildSizesInfo(type, geometry, instanceCount);

    buffers.scratchBuffer = Details::CreateAccelerationStructureBuffer(
            buildSizesInfo.buildScratchSize, vk::BufferUsageFlagBits::eStorageBuffer);

    buffers.storageBuffer = Details::CreateAccelerationStructureBuffer(
            buildSizesInfo.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR);

    const vk::AccelerationStructureCreateInfoKHR createInfo({}, buffers.storageBuffer, 0,
            buildSizesInfo.accelerationStructureSize, type, vk::DeviceAddress());

    const auto [result, tlas] = VulkanContext::device->Get().createAccelerationStructureKHR(createInfo);

    Assert(result == vk::Result::eSuccess);

    const vk::AccelerationStructureBuildGeometryInfoKHR buildInfo(
            type, Details::GetBuildFlags(type),
            vk::BuildAccelerationStructureModeKHR::eBuild,
            nullptr, tlas, 1, &geometry, nullptr,
            VulkanContext::device->GetAddress(buffers.scratchBuffer));

    const vk::AccelerationStructureBuildRangeInfoKHR rangeInfo(instanceCount, 0, 0, 0);
    const vk::AccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.buildAccelerationStructuresKHR({ buildInfo }, { pRangeInfo });
        });

    accelerationStructures.emplace(tlas, buffers);

    return tlas;
}

vk::AccelerationStructureKHR AccelerationStructureManager::CreateTlas(const TlasInstances& instances)
{
    constexpr vk::AccelerationStructureTypeKHR type = vk::AccelerationStructureTypeKHR::eTopLevel;

    AccelerationStructureBuffers buffers;

    buffers.sourceBuffer = Details::CreateEmptyInstanceBuffer(instances);

    const vk::AccelerationStructureGeometryInstancesDataKHR instancesData(
            false, VulkanContext::device->GetAddress(buffers.sourceBuffer));

    const vk::AccelerationStructureGeometryDataKHR geometryData(instancesData);

    const vk::AccelerationStructureGeometryKHR geometry(
            vk::GeometryTypeKHR::eInstances, geometryData,
            vk::GeometryFlagBitsKHR::eOpaque);

    const uint32_t instanceCount = static_cast<uint32_t>(instances.size());

    const vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo
            = Details::GetBuildSizesInfo(type, geometry, instanceCount);

    buffers.scratchBuffer = Details::CreateAccelerationStructureBuffer(
            buildSizesInfo.buildScratchSize, vk::BufferUsageFlagBits::eStorageBuffer);

    buffers.storageBuffer = Details::CreateAccelerationStructureBuffer(
            buildSizesInfo.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR);

    const vk::AccelerationStructureCreateInfoKHR createInfo({}, buffers.storageBuffer, 0,
            buildSizesInfo.accelerationStructureSize, type, vk::DeviceAddress());

    const auto [result, tlas] = VulkanContext::device->Get().createAccelerationStructureKHR(createInfo);

    Assert(result == vk::Result::eSuccess);

    accelerationStructures.emplace(tlas, buffers);

    return tlas;
}

void AccelerationStructureManager::BuildTlas(vk::CommandBuffer commandBuffer,
        vk::AccelerationStructureKHR tlas, const TlasInstances& instances)
{
    constexpr vk::AccelerationStructureTypeKHR type = vk::AccelerationStructureTypeKHR::eTopLevel;

    AccelerationStructureBuffers& buffers = accelerationStructures.at(tlas);

    BufferHelpers::UpdateBuffer(commandBuffer, buffers.sourceBuffer, GetByteView(instances),
            SyncScope::kWaitForNone, SyncScope::kAccelerationStructureWrite);

    const vk::AccelerationStructureGeometryInstancesDataKHR instancesData(
            false, VulkanContext::device->GetAddress(buffers.sourceBuffer));

    const vk::AccelerationStructureGeometryDataKHR geometryData(instancesData);

    const vk::AccelerationStructureGeometryKHR geometry(
            vk::GeometryTypeKHR::eInstances, geometryData,
            vk::GeometryFlagBitsKHR::eOpaque);

    const vk::AccelerationStructureBuildGeometryInfoKHR buildInfo(
            type, Details::GetBuildFlags(type),
            vk::BuildAccelerationStructureModeKHR::eBuild,
            nullptr, tlas, 1, &geometry, nullptr,
            VulkanContext::device->GetAddress(buffers.scratchBuffer));

    const uint32_t instanceCount = static_cast<uint32_t>(instances.size());

    const vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo
            = VulkanContext::device->Get().getAccelerationStructureBuildSizesKHR(
                    vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, { instanceCount });

    const BufferManager& bufferManager = *VulkanContext::bufferManager;

    Assert(buildSizesInfo.buildScratchSize <= bufferManager.GetBufferDescription(buffers.scratchBuffer).size);
    Assert(buildSizesInfo.accelerationStructureSize <= bufferManager.GetBufferDescription(buffers.storageBuffer).size);

    const vk::AccelerationStructureBuildRangeInfoKHR rangeInfo(instanceCount, 0, 0, 0);
    const vk::AccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

    commandBuffer.buildAccelerationStructuresKHR({ buildInfo }, { pRangeInfo });

    const PipelineBarrier pipelineBarrier{
        SyncScope::kAccelerationStructureWrite,
        SyncScope::kShaderRead
    };

    BufferHelpers::InsertPipelineBarrier(commandBuffer, buffers.storageBuffer, pipelineBarrier);
}

void AccelerationStructureManager::DestroyAccelerationStructure(vk::AccelerationStructureKHR accelerationStructure)
{
    const auto it = accelerationStructures.find(accelerationStructure);
    Assert(it != accelerationStructures.end());

    const AccelerationStructureBuffers& buffers = it->second;

    VulkanContext::device->Get().destroyAccelerationStructureKHR(accelerationStructure);

    if (buffers.sourceBuffer)
    {
        ResourceHelpers::DestroyResource(buffers.sourceBuffer);
    }

    ResourceHelpers::DestroyResource(buffers.scratchBuffer);
    ResourceHelpers::DestroyResource(buffers.storageBuffer);

    accelerationStructures.erase(it);
}
