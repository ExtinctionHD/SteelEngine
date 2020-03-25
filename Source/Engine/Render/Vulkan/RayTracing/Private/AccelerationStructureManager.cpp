#include "Engine/Render/Vulkan/RayTracing/AccelerationStructureManager.hpp"

#include "Engine/Render/RenderObject.hpp"
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
    vk::GeometryNV GetGeometry(const RenderObject &renderObject)
    {
        const vk::GeometryTrianglesNV triangles(renderObject.GetVertexBuffer()->buffer, 0,
                renderObject.GetVertexCount(), renderObject.GetVertexStride(), renderObject.GetVertexFormat().front(),
                renderObject.GetIndexBuffer()->buffer, 0, renderObject.GetIndexCount(), renderObject.GetIndexType());

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

    BufferHandle CreateScratchBuffer(vk::AccelerationStructureNV object)
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

        const BufferHandle buffer = VulkanContext::bufferManager->CreateBuffer(bufferDescription,
                BufferCreateFlags::kNone);

        return buffer;
    }

    vk::GeometryInstanceFlagsNV GetGeometryInstanceFlags()
    {
        return vk::GeometryInstanceFlagBitsNV::eTriangleFrontCounterclockwise
                | vk::GeometryInstanceFlagBitsNV::eForceOpaque;
    }

    BufferHandle CreateInstanceBuffer(const std::vector<GeometryInstance> &instances)
    {
        const uint32_t instanceCount = static_cast<uint32_t>(instances.size());

        std::vector<vk::GeometryInstanceNV> geometryInstances(instanceCount);
        for (uint32_t i = 0; i < instanceCount; ++i)
        {
            const GeometryInstance &instance = instances[i];
            const glm::mat4 transposedTransform = transpose(instance.transform);

            vk::GeometryInstanceNV &geometryInstance = geometryInstances[i];
            std::memcpy(geometryInstance.transform, &transposedTransform, sizeof(geometryInstance.transform));
            geometryInstance.customIndex = i;
            geometryInstance.mask = 0xFF;
            geometryInstance.flags = static_cast<uint32_t>(GetGeometryInstanceFlags());
            geometryInstance.hitGroupIndex = 0;

            VulkanContext::device->Get().getAccelerationStructureHandleNV(instance.blas,
                    sizeof(uint64_t), &geometryInstance.accelerationStructureHandle);
        }

        const BufferDescription bufferDescription{
            sizeof(vk::GeometryInstanceNV) * instanceCount,
            vk::BufferUsageFlagBits::eRayTracingNV | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const SyncScope blockedScope{
            vk::PipelineStageFlagBits::eAccelerationStructureBuildNV,
            vk::AccessFlagBits::eAccelerationStructureReadNV
        };

        const BufferHandle buffer = VulkanContext::bufferManager->CreateBuffer(
                bufferDescription, BufferCreateFlags::kNone,
                GetByteView(geometryInstances), blockedScope);

        return buffer;
    }
}

AccelerationStructureManager::~AccelerationStructureManager()
{
    for (auto &[tlas, instanceBuffer] : tlasInstanceBuffers)
    {
        VulkanContext::bufferManager->DestroyBuffer(instanceBuffer);
    }

    for (auto &[accelerationStructure, scratchBuffer] : accelerationStructures)
    {
        VulkanContext::bufferManager->DestroyBuffer(scratchBuffer);
        VulkanContext::memoryManager->DestroyAccelerationStructure(accelerationStructure);
    }
}

vk::AccelerationStructureNV AccelerationStructureManager::GenerateBlas(const RenderObject &renderObject)
{
    const vk::GeometryNV geometry = SASManager::GetGeometry(renderObject);
    const vk::AccelerationStructureInfoNV blasInfo = SASManager::GetBlasInfo(geometry);

    const vk::AccelerationStructureNV blas = VulkanContext::memoryManager->CreateAccelerationStructure({
        0, blasInfo
    });

    const BufferHandle scratchBuffer = SASManager::CreateScratchBuffer(blas);

    const DeviceCommands deviceCommands = [&blasInfo, &blas, &scratchBuffer](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.buildAccelerationStructureNV(blasInfo, nullptr, 0, false,
                    blas, nullptr, scratchBuffer->buffer, 0);
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

    const BufferHandle scratchBuffer = SASManager::CreateScratchBuffer(tlas);

    const BufferHandle instanceBuffer = SASManager::CreateInstanceBuffer(instances);

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.buildAccelerationStructureNV(tlasInfo, instanceBuffer->buffer, 0, false,
                    tlas, nullptr, scratchBuffer->buffer, 0);
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
