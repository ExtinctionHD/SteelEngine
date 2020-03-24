#include "Engine/Render/RenderObject.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"

namespace SRenderObject
{
    BufferHandle CreateVertexBuffer(const std::vector<Vertex> &vertices)
    {
        Assert(!vertices.empty());

        const BufferDescription description{
            vertices.size() * sizeof(Vertex),
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const SyncScope blockedScope{
            vk::PipelineStageFlagBits::eVertexInput,
            vk::AccessFlagBits::eVertexAttributeRead
        };

        const BufferHandle buffer = VulkanContext::bufferManager->CreateBuffer(description,
                BufferCreateFlagBits::eStagingBuffer, GetByteView(vertices), blockedScope);

        return buffer;
    }

    BufferHandle CreateIndexBuffer(const std::vector<uint32_t> &indices)
    {
        if (indices.empty())
        {
            return nullptr;
        }

        const BufferDescription description{
            indices.size() * sizeof(uint32_t),
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const SyncScope blockedScope{
            vk::PipelineStageFlagBits::eVertexInput,
            vk::AccessFlagBits::eIndexRead
        };

        const BufferHandle buffer = VulkanContext::bufferManager->CreateBuffer(description,
                BufferCreateFlagBits::eStagingBuffer, GetByteView(indices), blockedScope);

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
