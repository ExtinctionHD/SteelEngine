#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"
#include "Engine/Render/Vulkan/Resources/Texture.hpp"

struct VertexBuffer
{
    uint32_t vertexCount;
    VertexFormat vertexFormat;
    BufferHandle buffer;
};

struct IndexBuffer
{
    uint32_t indexCount;
    vk::IndexType indexType;
    BufferHandle buffer;
};

struct Material
{
    Texture baseColor;
};

struct RenderObject
{
    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
    Material material;
};
