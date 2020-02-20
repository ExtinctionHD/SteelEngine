#include "Engine/Render/Vulkan/RayTracing/BlasGenerator.hpp"

#include "Engine/Render/RenderObject.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"

#include "Utils/Helpers.hpp"

namespace SBlasGenerator
{
    vk::GeometryNV GetGeometry(const Mesh& mesh)
    {
        const vk::GeometryTrianglesNV triangles(mesh.vertexBuffer->buffer, 0, mesh.vertexCount,
            VulkanHelpers::CalculateVertexStride(mesh.vertexFormat), mesh.vertexFormat.front(),
            mesh.indexBuffer->buffer, 0, mesh.indexCount, mesh.indexType);

        const vk::GeometryDataNV geometryData(triangles);

        const vk::GeometryNV geometry(vk::GeometryTypeNV::eTriangles,
                geometryData, vk::GeometryFlagBitsNV::eOpaque);

        return geometry;
    }

    vk::AccelerationStructureInfoNV GetAccelerationStructureInfo(const vk::GeometryNV& geometry)
    {
        const vk::AccelerationStructureInfoNV accelerationStructureInfo(
            vk::AccelerationStructureTypeNV::eBottomLevel,
            vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace,
            0, 1, &geometry);

        return accelerationStructureInfo;
    }

    vk::AccelerationStructureNV CreateBlas(vk::Device device, const vk::GeometryNV &geometry)
    {
        const vk::AccelerationStructureCreateInfoNV createInfo(0, GetAccelerationStructureInfo(geometry));

        const auto [result, blas] = device.createAccelerationStructureNV(createInfo);
        Assert(result == vk::Result::eSuccess);

        return blas;
    }

    vk::DeviceMemory AllocateBlasMemory(const Device &device, vk::AccelerationStructureNV blas)
    {
        const vk::AccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo(
                vk::AccelerationStructureMemoryRequirementsTypeNV::eObject, blas);
        const vk::MemoryRequirements2 memoryRequirements
                = device.Get().getAccelerationStructureMemoryRequirementsNV(memoryRequirementsInfo);

        const vk::DeviceMemory memory = VulkanHelpers::AllocateDeviceMemory(device,
                memoryRequirements.memoryRequirements, vk::MemoryPropertyFlagBits::eDeviceLocal);

        const vk::BindAccelerationStructureMemoryInfoNV bindInfo(blas, memory, 0, 0, nullptr);

        const vk::Result result = device.Get().bindAccelerationStructureMemoryNV(bindInfo);
        Assert(result == vk::Result::eSuccess);

        return memory;
    }

    BufferDescription GetScratchBufferDescription(vk::Device device, vk::AccelerationStructureNV blas)
    {
        const vk::AccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo(
                vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch, blas);
        const vk::MemoryRequirements2 memoryRequirements
                = device.getAccelerationStructureMemoryRequirementsNV(memoryRequirementsInfo);

        const BufferDescription bufferDescription{
            memoryRequirements.memoryRequirements.size,
            vk::BufferUsageFlagBits::eRayTracingNV,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        return bufferDescription;
    }
}

BlasGenerator::BlasGenerator(std::shared_ptr<Device> aDevice, std::shared_ptr<BufferManager> aBufferManager)
    : device(aDevice)
    , bufferManager(aBufferManager)
{}

BlasGenerator::~BlasGenerator()
{
    for (auto &[blas, memory, scratchBuffer] : blasStorage)
    {
        device->Get().destroyAccelerationStructureNV(blas);
        device->Get().freeMemory(memory);
        bufferManager->DestroyBuffer(scratchBuffer);
    }
}

vk::AccelerationStructureNV BlasGenerator::GenerateBlas(const Mesh &mesh)
{
    const vk::GeometryNV geometry = SBlasGenerator::GetGeometry(mesh);

    const vk::AccelerationStructureNV blas = SBlasGenerator::CreateBlas(device->Get(), geometry);

    const vk::DeviceMemory memory = SBlasGenerator::AllocateBlasMemory(GetRef(device), blas);

    const BufferDescription scratchBufferDescription = SBlasGenerator::GetScratchBufferDescription(device->Get(), blas);
    const BufferHandle scratchBuffer = bufferManager->CreateBuffer(scratchBufferDescription);
    scratchBuffer->FreeCpuMemory();

    device->ExecuteOneTimeCommands([&geometry, &blas, &scratchBuffer](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.buildAccelerationStructureNV(SBlasGenerator::GetAccelerationStructureInfo(geometry),
                    nullptr, 0, false, blas, nullptr, scratchBuffer->buffer, 0);
        });

    blasStorage.push_back({ blas, memory, scratchBuffer });

    return blas;
}

void BlasGenerator::DestroyBlas(vk::AccelerationStructureNV blas)
{
    const auto it = std::find_if(blasStorage.begin(), blasStorage.end(), [blas](const auto& entry)
        {
            return entry.accelerationStructure == blas;
        });

    device->Get().destroyAccelerationStructureNV(blas);
    device->Get().freeMemory(it->memory);
    bufferManager->DestroyBuffer(it->scratchBuffer);

    blasStorage.erase(it);
}
