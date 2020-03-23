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

    Scene() = default;
    ~Scene();

    void AddNode(NodeHandle node);

private:
    std::shared_ptr<BufferManager> bufferManager;
    std::shared_ptr<TextureCache> textureCache;

    std::vector<NodeHandle> nodes;
};
