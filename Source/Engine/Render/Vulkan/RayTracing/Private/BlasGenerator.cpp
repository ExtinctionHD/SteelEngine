#include "Engine/Render/Vulkan/RayTracing/BlasGenerator.hpp"

BlasGenerator::BlasGenerator(std::shared_ptr<Device> aDevice)
    : device(aDevice)
{}

BlasGenerator::~BlasGenerator()
{
    for (auto &blas : accelerationStructures)
    {
        device->Get().destroyAccelerationStructureNV(blas);
    }
}

vk::AccelerationStructureNV BlasGenerator::GenerateBlas(BufferHandle vertexBuffer, uint32_t vertexCount,
        BufferHandle indexBuffer, uint32_t indexCount)
{
    (void)vertexBuffer;
    (void)vertexCount;
    (void)indexBuffer;
    (void)indexCount;

    return {};
}

void BlasGenerator::DestroyBlas(vk::AccelerationStructureNV blas)
{
    device->Get().destroyAccelerationStructureNV(blas);
    accelerationStructures.remove(blas);
}
