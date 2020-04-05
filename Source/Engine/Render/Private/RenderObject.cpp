#include "Engine/Render/RenderObject.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"

namespace SRenderObject
{
    vk::Buffer CreateVertexBuffer(const std::vector<Vertex> &vertices)
    {
        Assert(!vertices.empty());

        const vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;

        const BufferDescription description{
            vertices.size() * sizeof(Vertex), usage,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(
                description, BufferCreateFlagBits::eStagingBuffer);

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                VulkanContext::bufferManager->UpdateBuffer(commandBuffer, buffer, GetByteView(vertices));

                const PipelineBarrier barrier{
                    SyncScope::kTransferWrite,
                    SyncScope::kVerticesRead | SyncScope::kRayTracingShaderRead
                };

                BufferHelpers::SetupPipelineBarrier(commandBuffer, buffer, barrier);
            });


        return buffer;
    }

    vk::Buffer CreateIndexBuffer(const std::vector<uint32_t> &indices)
    {
        if (indices.empty())
        {
            return nullptr;
        }

        const vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eIndexBuffer
                | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;

        const BufferDescription description{
            indices.size() * sizeof(uint32_t), usage,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(
                description, BufferCreateFlagBits::eStagingBuffer);

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                VulkanContext::bufferManager->UpdateBuffer(commandBuffer, buffer, GetByteView(indices));

                const PipelineBarrier barrier{
                    SyncScope::kTransferWrite,
                    SyncScope::kIndicesRead | SyncScope::kRayTracingShaderRead
                };

                BufferHelpers::SetupPipelineBarrier(commandBuffer, buffer, barrier);
            });


        return buffer;
    }
}

const VertexFormat Vertex::kFormat{
    vk::Format::eR32G32B32Sfloat,
    vk::Format::eR32G32B32Sfloat,
    vk::Format::eR32G32B32Sfloat,
    vk::Format::eR32G32Sfloat,
};

RenderObject::RenderObject(const std::vector<Vertex> &vertices_,
        const std::vector<uint32_t> &indices_,
        const Material &material_)
    : vertices(vertices_)
    , indices(indices_)
    , material(material_)
{
    vertexBuffer = SRenderObject::CreateVertexBuffer(vertices);
    indexBuffer = SRenderObject::CreateIndexBuffer(indices);
}
