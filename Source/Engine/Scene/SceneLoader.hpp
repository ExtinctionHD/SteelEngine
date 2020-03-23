#pragma once

#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"
#include "Engine/Render/Vulkan/Resources/TextureCache.hpp"
#include "Engine/Scene/Node.hpp"

class Filepath;

namespace tinygltf
{
    class Model;
    class Node;
    struct Primitive;
}

class SceneLoader
{
public:
    SceneLoader(std::shared_ptr<BufferManager> bufferManager_, std::shared_ptr<TextureCache> textureCache_);

    std::unique_ptr<Scene> LoadFromFile(const Filepath &path) const;

private:
    std::shared_ptr<BufferManager> bufferManager;
    std::shared_ptr<TextureCache> textureCache;

    NodeHandle CreateNode(const tinygltf::Model &gltfModel, const tinygltf::Node &gltfNode,
            const Scene &scene, NodeHandle parent) const;

    std::vector<RenderObject> CreateRenderObjects(const tinygltf::Model &gltfModel,
            const tinygltf::Node &gltfNode) const;

    RenderObject CreateRenderObject(const tinygltf::Model &gltfModel,
            const tinygltf::Primitive &gltfPrimitive) const;
};
