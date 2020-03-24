#pragma warning(push, 0)
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#include <tiny_gltf.h>
#pragma warning(pop)

#include "Engine/Scene/SceneLoader.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Engine/Scene/Scene.hpp"
#include "Engine/EngineHelpers.hpp"

namespace SSceneLoader
{
    tinygltf::Model LoadModel(const Filepath &path)
    {
        using namespace tinygltf;

        TinyGLTF loader;

        Model model;
        std::string errors;
        std::string warnings;

        const bool result = loader.LoadASCIIFromFile(&model, &errors, &warnings, path.GetAbsolute());

        if (!warnings.empty())
        {
            LogW << "Scene loaded with warnings:\n" << warnings;
        }

        if (!errors.empty())
        {
            LogE << "Failed to load scene:\n" << errors;
        }

        Assert(result);

        return model;
    }

    glm::mat4 CreateTransform(tinygltf::Node gltfNode)
    {
        glm::mat4 transform = Matrix4::kIdentity;

        if (!gltfNode.translation.empty())
        {
            const glm::vec3 translation(gltfNode.translation[0], gltfNode.translation[1], gltfNode.translation[2]);
            transform = glm::translate(transform, translation);
        }

        if (!gltfNode.rotation.empty())
        {
            const glm::quat rotation(
                    static_cast<float>(gltfNode.rotation[0]),
                    static_cast<float>(gltfNode.rotation[1]),
                    static_cast<float>(gltfNode.rotation[2]),
                    static_cast<float>(gltfNode.rotation[3]));

            transform = glm::toMat4(rotation) * transform;
        }

        if (!gltfNode.scale.empty())
        {
            const glm::vec3 scale(gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]);
            transform = glm::scale(transform, scale);
        }

        return transform;
    }

    RenderObject CreateRenderObject(const tinygltf::Model &gltfModel,
            const tinygltf::Primitive &gltfPrimitive)
    {
        struct VertexAttribute
        {
            std::string name;
            uint32_t size;
            std::optional<tinygltf::Accessor> gltfAccessor;
        };

        std::vector<VertexAttribute> attributes{
            { "POSITION", sizeof(glm::vec3), std::nullopt },
            { "NORMAL", sizeof(glm::vec3), std::nullopt },
            { "TANGENT", sizeof(glm::vec3), std::nullopt },
            { "TEXCOORD_0", sizeof(glm::vec2), std::nullopt }
        };

        for (const auto &[name, accessorIndex] : gltfPrimitive.attributes)
        {
            const auto pred = [&name](const auto &attribute)
                {
                    return attribute.name == name;
                };

            const auto it = std::find_if(attributes.begin(), attributes.end(), pred);

            if (it != attributes.end())
            {
                it->gltfAccessor = gltfModel.accessors[accessorIndex];
            }
        }

        Assert(attributes.front().gltfAccessor.has_value());

        const Vertex defaultVertex{
            Vector3::kZero,
            Direction::kUp,
            Vector3::kZero,
            glm::vec2(0.0f, 0.0f)
        };

        std::vector<Vertex> vertices(attributes.front().gltfAccessor->count, defaultVertex);

        uint32_t attributeOffset = 0;
        for (const auto &[name, size, gltfAccessor] : attributes)
        {
            if (gltfAccessor.has_value())
            {
                Assert(gltfAccessor->count == vertices.size());

                const tinygltf::BufferView &gltfBufferView = gltfModel.bufferViews[gltfAccessor->bufferView];
                const tinygltf::Buffer &gltfBuffer = gltfModel.buffers[gltfBufferView.buffer];

                const ByteView gltfBufferData = GetByteView(gltfBuffer.data);
                const ByteAccess vertexData = GetByteAccess(vertices);

                uint32_t gltfBufferStride = size;
                if (gltfBufferView.byteStride != 0)
                {
                    gltfBufferStride = static_cast<uint32_t>(gltfBufferView.byteStride);
                }

                for (uint32_t i = 0; i < vertices.size(); ++i)
                {
                    uint8_t *dst = vertexData.data + i * sizeof(Vertex) + attributeOffset;
                    const uint8_t *src = gltfBufferData.data + gltfBufferView.byteOffset + i * gltfBufferStride;

                    std::memcpy(dst, src, size);
                }
            }

            attributeOffset += size;
        }
        const RenderObject renderObject(vertices, {}, Material{});

        return renderObject;
    }

    std::vector<RenderObject> CreateRenderObjects(const tinygltf::Model &gltfModel,
            const tinygltf::Node &gltfNode)
    {
        std::vector<RenderObject> renderObjects;

        if (gltfNode.mesh != -1)
        {
            const tinygltf::Mesh &gltfMesh = gltfModel.meshes[gltfNode.mesh];

            renderObjects.reserve(gltfMesh.primitives.size());

            for (const auto &gltfPrimitive : gltfMesh.primitives)
            {
                renderObjects.push_back(CreateRenderObject(gltfModel, gltfPrimitive));
            }
        }

        return renderObjects;
    }
}

std::unique_ptr<Scene> SceneLoader::LoadFromFile(const Filepath &path)
{
    const tinygltf::Model gltfModel = SSceneLoader::LoadModel(path);

    Assert(!gltfModel.scenes.empty());

    std::unique_ptr<Scene> scene = std::make_unique<Scene>();

    for (const auto &nodeIndex : gltfModel.scenes.front().nodes)
    {
        scene->AddNode(CreateNode(gltfModel,
                gltfModel.nodes[nodeIndex], GetRef(scene), nullptr));
    }

    return scene;
}

NodeHandle SceneLoader::CreateNode(const tinygltf::Model &gltfModel,
        const tinygltf::Node &gltfNode, const Scene &scene, NodeHandle parent)
{
    Node *node = new Node(scene);
    node->name = gltfNode.name;
    node->transform = SSceneLoader::CreateTransform(gltfNode);
    node->renderObjects = SSceneLoader::CreateRenderObjects(gltfModel, gltfNode);

    node->parent = parent;
    node->children.reserve(gltfNode.children.size());

    for (const auto &nodeIndex : gltfNode.children)
    {
        node->children.push_back(CreateNode(gltfModel, gltfModel.nodes[nodeIndex], scene, node));
    }

    return node;
}
