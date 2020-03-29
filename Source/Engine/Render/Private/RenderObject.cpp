#include "Engine/Render/RenderObject.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"

namespace SRenderObject
{
    vk::Buffer CreateVertexBuffer(const std::vector<Vertex> &vertices)
    {
        Assert(!vertices.empty());

        const BufferDescription description{
            vertices.size() * sizeof(Vertex),
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(
                description, BufferCreateFlagBits::eStagingBuffer);

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                VulkanContext::bufferManager->UpdateBuffer(commandBuffer, buffer, GetByteView(vertices));

                const BufferRange range{ 0, description.size };

                const PipelineBarrier barrier{
                    SyncScope::kTransferWrite,
                    SyncScope::kVerticesRead
                };

                BufferHelpers::SetupPipelineBarrier(commandBuffer, buffer, range, barrier);
            });


        return buffer;
    }

    vk::Buffer CreateIndexBuffer(const std::vector<uint32_t> &indices)
    {
        if (indices.empty())
        {
            return nullptr;
        }

        const BufferDescription description{
            indices.size() * sizeof(uint32_t),
            vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(
                description, BufferCreateFlagBits::eStagingBuffer);

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                VulkanContext::bufferManager->UpdateBuffer(commandBuffer, buffer, GetByteView(indices));

                const BufferRange range{ 0, description.size };

                const PipelineBarrier barrier{
                    SyncScope::kTransferWrite,
                    SyncScope::kIndicesRead
                };

                BufferHelpers::SetupPipelineBarrier(commandBuffer, buffer, range, barrier);
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
