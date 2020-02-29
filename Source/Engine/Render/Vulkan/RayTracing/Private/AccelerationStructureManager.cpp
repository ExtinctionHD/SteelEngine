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

    vk::AccelerationStructureNV CreateAccelerationStructureObject(vk::Device device,
            const vk::AccelerationStructureInfoNV &info)
    {
        const vk::AccelerationStructureCreateInfoNV createInfo(0, info);

        const auto [result, accelerationStructure] = device.createAccelerationStructureNV(createInfo);
        Assert(result == vk::Result::eSuccess);

        return accelerationStructure;
    }

    vk::DeviceMemory AllocateAccelerationStructureMemory(const Device &device,
            vk::AccelerationStructureNV object)
    {
        const vk::AccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo(
                vk::AccelerationStructureMemoryRequirementsTypeNV::eObject, object);
        const vk::MemoryRequirements2 memoryRequirements
                = device.Get().getAccelerationStructureMemoryRequirementsNV(memoryRequirementsInfo);

        const vk::DeviceMemory memory = VulkanHelpers::AllocateDeviceMemory(device,
                memoryRequirements.memoryRequirements, vk::MemoryPropertyFlagBits::eDeviceLocal);

        const vk::BindAccelerationStructureMemoryInfoNV bindInfo(object, memory, 0, 0, nullptr);

        const vk::Result result = device.Get().bindAccelerationStructureMemoryNV(bindInfo);
        Assert(result == vk::Result::eSuccess);

        return memory;
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

        const BufferHandle buffer = bufferManager.CreateBuffer(bufferDescription);
        buffer->FreeCpuMemory();

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

        const BufferHandle buffer = bufferManager.CreateBuffer(bufferDescription, geometryInstances);

        return buffer;
    }
}

AccelerationStructureManager::AccelerationStructureManager(std::shared_ptr<Device> aDevice,
        std::shared_ptr<BufferManager> aBufferManager)
    : device(aDevice)
    , bufferManager(aBufferManager)
{}

AccelerationStructureManager::~AccelerationStructureManager()
{
    for (auto &[tlasObject, instanceBuffer] : tlasInstanceBuffers)
    {
        bufferManager->DestroyBuffer(instanceBuffer);
    }

    for (auto &[object, memory, scratchBuffer] : accelerationStructures)
    {
        device->Get().destroyAccelerationStructureNV(object);
        device->Get().freeMemory(memory);
        bufferManager->DestroyBuffer(scratchBuffer);
    }
}

vk::AccelerationStructureNV AccelerationStructureManager::GenerateBlas(const Mesh &mesh)
{
    const vk::GeometryNV geometry = SASManager::GetGeometry(mesh);
    const vk::AccelerationStructureInfoNV blasInfo = SASManager::GetBlasInfo(geometry);

    const AccelerationStructure blas = CreateAccelerationStructure(blasInfo);

    device->ExecuteOneTimeCommands([&blasInfo, &blas](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.buildAccelerationStructureNV(blasInfo, nullptr, 0, false,
                    blas.object, nullptr, blas.scratchBuffer->buffer, 0);
        });

    accelerationStructures.push_back(blas);

    return blas.object;
}

vk::AccelerationStructureNV AccelerationStructureManager::GenerateTlas(
        const std::vector<GeometryInstance> &instances)
{
    const uint32_t instanceCount = static_cast<uint32_t>(instances.size());

    const vk::AccelerationStructureInfoNV tlasInfo = SASManager::GetTlasInfo(instanceCount);

    const AccelerationStructure tlas = CreateAccelerationStructure(tlasInfo);

    const BufferHandle instanceBuffer = SASManager::CreateInstanceBuffer(device->Get(),
            GetRef(bufferManager), instances);

    device->ExecuteOneTimeCommands([&tlasInfo, &tlas, &instanceBuffer](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.buildAccelerationStructureNV(tlasInfo, instanceBuffer->buffer, 0, false,
                    tlas.object, nullptr, tlas.scratchBuffer->buffer, 0);
        });

    accelerationStructures.push_back(tlas);
    tlasInstanceBuffers.emplace(tlas.object, instanceBuffer);

    return tlas.object;
}

void AccelerationStructureManager::DestroyAccelerationStructure(vk::AccelerationStructureNV accelerationStructure)
{
    const auto tlasIt = tlasInstanceBuffers.find(accelerationStructure);

    if (tlasIt != tlasInstanceBuffers.end())
    {
        bufferManager->DestroyBuffer(tlasIt->second);
        tlasInstanceBuffers.erase(tlasIt);
    }

    const auto pred = [accelerationStructure](const AccelerationStructure &as)
        {
            return as.object == accelerationStructure;
        };

    const auto it = std::find_if(accelerationStructures.begin(), accelerationStructures.end(), pred);

    device->Get().destroyAccelerationStructureNV(accelerationStructure);
    device->Get().freeMemory(it->memory);
    bufferManager->DestroyBuffer(it->scratchBuffer);
    accelerationStructures.erase(it);
}

AccelerationStructureManager::AccelerationStructure AccelerationStructureManager::CreateAccelerationStructure(
        const vk::AccelerationStructureInfoNV &info) const
{
    const vk::AccelerationStructureNV object = SASManager::CreateAccelerationStructureObject(
            device->Get(), info);

    const vk::DeviceMemory memory = SASManager::AllocateAccelerationStructureMemory(GetRef(device), object);

    const BufferHandle scratchBuffer = SASManager::CreateScratchBuffer(device->Get(),
            GetRef(bufferManager), object);

    return { object, memory, scratchBuffer };
}
