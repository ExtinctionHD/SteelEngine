#include "Engine/Render/Vulkan/RayTracing/AccelerationStructureManager.hpp"

#include "Engine/Render/RenderObject.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"

#include "Utils/Helpers.hpp"

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
    vk::GeometryNV GetGeometry(const Mesh &mesh)
    {
        const vk::GeometryTrianglesNV triangles(mesh.vertexBuffer->buffer, 0, mesh.vertexCount,
                VulkanHelpers::CalculateVertexStride(mesh.vertexFormat), mesh.vertexFormat.front(),
                mesh.indexBuffer->buffer, 0, mesh.indexCount, mesh.indexType);

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

    BufferHandle CreateScratchBuffer(vk::Device device, BufferManager &bufferManager,
            vk::AccelerationStructureNV object)
    {
        const vk::AccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo(
                vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch, object);
        const vk::MemoryRequirements2 memoryRequirements
                = device.getAccelerationStructureMemoryRequirementsNV(memoryRequirementsInfo);

        const BufferDescription bufferDescription{
            memoryRequirements.memoryRequirements.size,
            vk::BufferUsageFlagBits::eRayTracingNV,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const BufferHandle buffer = bufferManager.CreateBuffer(bufferDescription, BufferAccessFlags::kNone);

        return buffer;
    }

    vk::GeometryInstanceFlagsNV GetGeometryInstanceFlags()
    {
        return vk::GeometryInstanceFlagBitsNV::eTriangleFrontCounterclockwise
                | vk::GeometryInstanceFlagBitsNV::eForceOpaque;
    }

    BufferHandle CreateInstanceBuffer(vk::Device device, BufferManager &bufferManager,
            const std::vector<GeometryInstance> &instances)
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
            geometryInstance.mask = 0xff;
            geometryInstance.flags = static_cast<uint32_t>(GetGeometryInstanceFlags());
            geometryInstance.hitGroupIndex = 0;
            device.getAccelerationStructureHandleNV(instance.blas, sizeof(uint64_t),
                    &geometryInstance.accelerationStructureHandle);
        }

        const BufferDescription bufferDescription{
            sizeof(vk::GeometryInstanceNV) * instanceCount,
            vk::BufferUsageFlagBits::eRayTracingNV | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const BufferHandle buffer = bufferManager.CreateBuffer(bufferDescription,
                BufferAccessFlags::kNone, std::move(geometryInstances));

        return buffer;
    }
}

AccelerationStructureManager::AccelerationStructureManager(std::shared_ptr<Device> device_,
        std::shared_ptr<MemoryManager> memoryManager_, std::shared_ptr<BufferManager> bufferManager_)
    : device(device_)
    , memoryManager(memoryManager_)
    , bufferManager(bufferManager_)
{}

AccelerationStructureManager::~AccelerationStructureManager()
{
    for (auto &[tlas, instanceBuffer] : tlasInstanceBuffers)
    {
        bufferManager->DestroyBuffer(instanceBuffer);
    }

    for (auto &[accelerationStructure, scratchBuffer] : accelerationStructures)
    {
        bufferManager->DestroyBuffer(scratchBuffer);
        memoryManager->DestroyAccelerationStructure(accelerationStructure);
    }
}

vk::AccelerationStructureNV AccelerationStructureManager::GenerateBlas(const Mesh &mesh)
{
    const vk::GeometryNV geometry = SASManager::GetGeometry(mesh);
    const vk::AccelerationStructureInfoNV blasInfo = SASManager::GetBlasInfo(geometry);

    const vk::AccelerationStructureNV blas = memoryManager->CreateAccelerationStructure({ 0, blasInfo });

    const BufferHandle scratchBuffer = SASManager::CreateScratchBuffer(device->Get(),
            GetRef(bufferManager), blas);

    device->ExecuteOneTimeCommands([&blasInfo, &blas, &scratchBuffer](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.buildAccelerationStructureNV(blasInfo, nullptr, 0, false,
                    blas, nullptr, scratchBuffer->buffer, 0);
        });

    accelerationStructures.emplace(blas, scratchBuffer);

    return blas;
}

vk::AccelerationStructureNV AccelerationStructureManager::GenerateTlas(
        const std::vector<GeometryInstance> &instances)
{
    const uint32_t instanceCount = static_cast<uint32_t>(instances.size());

    const vk::AccelerationStructureInfoNV tlasInfo = SASManager::GetTlasInfo(instanceCount);

    const vk::AccelerationStructureNV tlas = memoryManager->CreateAccelerationStructure({ 0, tlasInfo });

    const BufferHandle scratchBuffer = SASManager::CreateScratchBuffer(device->Get(),
            GetRef(bufferManager), tlas);

    const BufferHandle instanceBuffer = SASManager::CreateInstanceBuffer(device->Get(),
            GetRef(bufferManager), instances);

    device->ExecuteOneTimeCommands([&tlasInfo, &tlas, &instanceBuffer, &scratchBuffer](vk::CommandBuffer commandBuffer)
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
        bufferManager->DestroyBuffer(tlasIt->second);
        tlasInstanceBuffers.erase(tlasIt);
    }

    const auto it = accelerationStructures.find(accelerationStructure);
    Assert(it != accelerationStructures.end());

    bufferManager->DestroyBuffer(it->second);
    memoryManager->DestroyAccelerationStructure(accelerationStructure);

    accelerationStructures.erase(it);
}
