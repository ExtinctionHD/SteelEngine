#include "Engine/Render/Vulkan/RayTracing/AccelerationStructureManager.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

namespace vk
{
    struct GeometryInstanceNV
    {
        float transform[12];
        uint32_t customIndex : 24;
        uint32_t mask : 8;
        uint32_t hitGroupIndex : 24;
        uint32_t flags : 8;
        uint64_t accelerationStructureHandle;
    };
}

namespace SASManager
{
    vk::GeometryNV GetGeometry(const GeometryVertices &vertices, const GeometryIndices &indices)
    {
        const vk::GeometryTrianglesNV triangles(vertices.buffer, 0,
                vertices.count, vertices.stride, vertices.format,
                indices.buffer, 0, indices.count, indices.type);

        const vk::GeometryDataNV geometryData(triangles);

        const vk::GeometryNV geometry(vk::GeometryTypeNV::eTriangles,
                geometryData, vk::GeometryFlagBitsNV::eOpaque);

        return geometry;
    }

    vk::AccelerationStructureInfoNV GetBlasInfo(const vk::GeometryNV &geometry)
    {
        const vk::AccelerationStructureInfoNV blasInfo(
                vk::AccelerationStructureTypeNV::eBottomLevel,
                vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace,
                0, 1, &geometry);

        return blasInfo;
    }

    vk::AccelerationStructureInfoNV GetTlasInfo(uint32_t instanceCount)
    {
        const vk::AccelerationStructureInfoNV tlasInfo(
                vk::AccelerationStructureTypeNV::eTopLevel,
                vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace,
                instanceCount, 0, nullptr);

        return tlasInfo;
    }

    vk::Buffer CreateScratchBuffer(vk::AccelerationStructureNV object)
    {
        const vk::Device device = VulkanContext::device->Get();

        const vk::AccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo(
                vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch, object);

        const vk::MemoryRequirements2 memoryRequirements
                = device.getAccelerationStructureMemoryRequirementsNV(memoryRequirementsInfo);

        const BufferDescription bufferDescription{
            memoryRequirements.memoryRequirements.size,
            vk::BufferUsageFlagBits::eRayTracingNV,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(bufferDescription,
                BufferCreateFlags::kNone);

        return buffer;
    }

    vk::GeometryInstanceFlagsNV GetGeometryInstanceFlags()
    {
        return vk::GeometryInstanceFlagBitsNV::eTriangleFrontCounterclockwise
                | vk::GeometryInstanceFlagBitsNV::eForceOpaque;
    }

    vk::Buffer CreateInstanceBuffer(const std::vector<GeometryInstance> &instances)
    {
        const uint32_t instanceCount = static_cast<uint32_t>(instances.size());

        std::vector<vk::GeometryInstanceNV> vkInstances(instanceCount);
        for (uint32_t i = 0; i < instanceCount; ++i)
        {
            const GeometryInstance &instance = instances[i];
            const glm::mat4 transposedTransform = transpose(instance.transform);

            vk::GeometryInstanceNV &vkInstance = vkInstances[i];
            std::memcpy(vkInstance.transform, &transposedTransform, sizeof(vkInstance.transform));
            vkInstance.customIndex = i;
            vkInstance.mask = 0xFF;
            vkInstance.flags = static_cast<uint32_t>(GetGeometryInstanceFlags());
            vkInstance.hitGroupIndex = 0;

            VulkanContext::device->Get().getAccelerationStructureHandleNV(instance.blas,
                    sizeof(uint64_t), &vkInstance.accelerationStructureHandle);
        }

        const BufferDescription bufferDescription{
            sizeof(vk::GeometryInstanceNV) * instanceCount,
            vk::BufferUsageFlagBits::eRayTracingNV | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(
                bufferDescription, BufferCreateFlagBits::eStagingBuffer);

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
}

vk::AccelerationStructureNV AccelerationStructureManager::GenerateBlas(const GeometryVertices &vertices,
        const GeometryIndices &indices)
{
    const vk::GeometryNV geometry = SASManager::GetGeometry(vertices, indices);
    const vk::AccelerationStructureInfoNV blasInfo = SASManager::GetBlasInfo(geometry);

    const vk::AccelerationStructureCreateInfoNV createInfo(0, blasInfo);

    const vk::AccelerationStructureNV blas = VulkanContext::memoryManager->CreateAccelerationStructure(createInfo);

    const vk::Buffer scratchBuffer = SASManager::CreateScratchBuffer(blas);

    const DeviceCommands deviceCommands = [&](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.buildAccelerationStructureNV(blasInfo,
                    nullptr, 0, false, blas, nullptr, scratchBuffer, 0);
        };

    VulkanContext::device->ExecuteOneTimeCommands(deviceCommands);

    accelerationStructures.emplace(blas, scratchBuffer);

    return blas;
}

vk::AccelerationStructureNV AccelerationStructureManager::GenerateTlas(
        const std::vector<GeometryInstance> &instances)
{
    const uint32_t instanceCount = static_cast<uint32_t>(instances.size());

    const vk::AccelerationStructureInfoNV tlasInfo = SASManager::GetTlasInfo(instanceCount);

    const vk::AccelerationStructureCreateInfoNV createInfo{ 0, tlasInfo };

    const vk::AccelerationStructureNV tlas = VulkanContext::memoryManager->CreateAccelerationStructure(createInfo);

    const vk::Buffer scratchBuffer = SASManager::CreateScratchBuffer(tlas);

    const vk::Buffer instanceBuffer = SASManager::CreateInstanceBuffer(instances);

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.buildAccelerationStructureNV(tlasInfo, instanceBuffer, 0, false,
                    tlas, nullptr, scratchBuffer, 0);
        });

    accelerationStructures.emplace(tlas, scratchBuffer);
    tlasInstanceBuffers.emplace(tlas, instanceBuffer);

    return tlas;
}

void AccelerationStructureManager::DestroyAccelerationStructure(vk::AccelerationStructureNV accelerationStructure)
{
    const auto tlasIt = tlasInstanceBuffers.find(accelerationStructure);

    if (tlasIt != tlasInstanceBuffers.end())
    {
        VulkanContext::bufferManager->DestroyBuffer(tlasIt->second);
        tlasInstanceBuffers.erase(tlasIt);
    }

    const auto it = accelerationStructures.find(accelerationStructure);
    Assert(it != accelerationStructures.end());

    VulkanContext::bufferManager->DestroyBuffer(it->second);
    VulkanContext::memoryManager->DestroyAccelerationStructure(accelerationStructure);

    accelerationStructures.erase(it);
}
