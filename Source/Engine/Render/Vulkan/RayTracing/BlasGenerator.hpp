#pragma once

#include "Engine/Render/Vulkan/Resources/Buffer.hpp"
#include "Engine/Render/Vulkan/Device.hpp"

class BlasGenerator
{
public:
    BlasGenerator(std::shared_ptr<Device> aDevice);
    ~BlasGenerator();

    vk::AccelerationStructureNV GenerateBlas(BufferHandle vertexBuffer, uint32_t vertexCount,
            BufferHandle indexBuffer, uint32_t indexCount);

    void DestroyBlas(vk::AccelerationStructureNV blas);

private:
    std::shared_ptr<Device> device;

    std::list<vk::AccelerationStructureNV> accelerationStructures;
};
