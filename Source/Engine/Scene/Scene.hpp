#pragma once

#include "Engine/Scene/Node.hpp"
#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"
#include "Engine/Render/Vulkan/Resources/TextureCache.hpp"

class Filepath;

namespace tinygltf
{
    class Model;
    class Node;
    struct Primitive;
}

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 texCoord;
};

class Scene
{
public:
    static const VertexFormat kVertexFormat;
    static const vk::IndexType kIndexType;

    Scene(std::shared_ptr<BufferManager> bufferManager_, std::shared_ptr<TextureCache> textureCache_);
    ~Scene();

    void LoadFromFile(const Filepath &path);

private:
    std::shared_ptr<BufferManager> bufferManager;
    std::shared_ptr<TextureCache> textureCache;

    std::vector<NodeHandle> nodes;

    NodeHandle ProcessNode(const tinygltf::Model &gltfModel,
            const tinygltf::Node &gltfNode, NodeHandle parent) const;

    std::vector<RenderObject> CreateRenderObjects(const tinygltf::Model &gltfModel,
            const tinygltf::Node &gltfNode) const;

    RenderObject CreateRenderObject(const tinygltf::Model &gltfModel,
            const tinygltf::Primitive &gltfPrimitive) const;
};
