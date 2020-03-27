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
    tinygltf::Model LoadGltfModelFromFile(const Filepath &path)
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
}

using NodeCreator = std::function<Node *(const Scene &)>;

class GltfParser
{
public:
    GltfParser(const Filepath &path, const NodeCreator &nodeCreator_)
        : nodeCreator(nodeCreator_)
    {
        gltfModel = SSceneLoader::LoadGltfModelFromFile(path);
        Assert(!gltfModel.scenes.empty());
    }

    std::unique_ptr<Scene> CreateScene() const
    {
        std::unique_ptr<Scene> scene = std::make_unique<Scene>();

        for (const auto &nodeIndex : gltfModel.scenes.front().nodes)
        {
            scene->AddNode(CreateNode(gltfModel.nodes[nodeIndex], GetRef(scene), nullptr));
        }

        return scene;
    }

private:
    struct VertexAttribute
    {
        std::string name;
        uint32_t size;
        int index;
        std::optional<tinygltf::Accessor> gltfAccessor;
    };

    NodeCreator nodeCreator;

    tinygltf::Model gltfModel;

    NodeHandle CreateNode(const tinygltf::Node &gltfNode, const Scene &scene, NodeHandle parent) const
    {
        Node *node = nodeCreator(scene);
        node->name = gltfNode.name;
        node->transform = RetrieveTransform(gltfNode);
        node->renderObjects = CreateRenderObjects(gltfNode);
        node->parent = parent;
        node->children.reserve(gltfNode.children.size());

        for (const auto &nodeIndex : gltfNode.children)
        {
            node->children.push_back(CreateNode(gltfModel.nodes[nodeIndex], scene, node));
        }

        return node;
    }

    glm::mat4 RetrieveTransform(const tinygltf::Node &gltfNode) const
    {
        glm::mat4 transform = Matrix4::kIdentity;

        if (!gltfNode.translation.empty())
        {
            const glm::vec3 translation(gltfNode.translation[0], gltfNode.translation[1], gltfNode.translation[2]);

            transform = glm::translate(transform, translation);
        }

        if (!gltfNode.rotation.empty())
        {
            glm::quat rotation(
                    static_cast<float>(gltfNode.rotation[0]),
                    static_cast<float>(gltfNode.rotation[1]),
                    static_cast<float>(gltfNode.rotation[2]),
                    static_cast<float>(gltfNode.rotation[3]));

            rotation = glm::rotate(rotation, glm::pi<float>(), Direction::kRight);

            transform = glm::toMat4(rotation) * transform;
        }

        if (!gltfNode.scale.empty())
        {
            const glm::vec3 scale(gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]);

            transform = glm::scale(transform, scale);
        }

        return transform;
    }

    std::vector<RenderObject> CreateRenderObjects(const tinygltf::Node &gltfNode) const
    {
        std::vector<RenderObject> renderObjects;

        if (gltfNode.mesh != -1)
        {
            const tinygltf::Mesh &gltfMesh = gltfModel.meshes[gltfNode.mesh];

            renderObjects.reserve(gltfMesh.primitives.size());

            for (const auto &gltfPrimitive : gltfMesh.primitives)
            {
                renderObjects.push_back(CreateRenderObject(gltfPrimitive));
            }
        }

        return renderObjects;
    }

    RenderObject CreateRenderObject(const tinygltf::Primitive &gltfPrimitive) const
    {
        const std::vector<Vertex> vertices = RetrieveVertices(gltfPrimitive);
        const std::vector<uint32_t> indices = RetrieveIndices(gltfPrimitive);

        return RenderObject(vertices, indices, Material{});
    }

    std::vector<Vertex> RetrieveVertices(const tinygltf::Primitive &gltfPrimitive) const
    {
        const std::vector<VertexAttribute> attributes = RetrieveVertexAttributes(gltfPrimitive);

        Assert(attributes.front().gltfAccessor.has_value());

        const Vertex defaultVertex{
            Vector3::kZero,
            Direction::kUp,
            Vector3::kZero,
            glm::vec2(0.0f, 0.0f)
        };

        std::vector<Vertex> vertices(attributes.front().gltfAccessor->count, defaultVertex);

        uint32_t attributeOffset = 0;

        for (const auto &[name, size, index, gltfAccessor] : attributes)
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
                const size_t gltfBufferOffset = gltfAccessor->byteOffset + gltfBufferView.byteOffset;

                for (uint32_t i = 0; i < vertices.size(); ++i)
                {
                    uint8_t *dst = vertexData.data + i * sizeof(Vertex) + attributeOffset;
                    const uint8_t *src = gltfBufferData.data + gltfBufferOffset + i * gltfBufferStride;

                    std::memcpy(dst, src, size);
                }
            }

            attributeOffset += size;
        }

        return vertices;
    }

    std::vector<uint32_t> RetrieveIndices(const tinygltf::Primitive &gltfPrimitive) const
    {
        std::vector<uint32_t> indices;

        if (gltfPrimitive.indices != -1)
        {
            const tinygltf::Accessor &gltfAccessor = gltfModel.accessors[gltfPrimitive.indices];
            const tinygltf::BufferView &gltfBufferView = gltfModel.bufferViews[gltfAccessor.bufferView];
            const tinygltf::Buffer &gltfBuffer = gltfModel.buffers[gltfBufferView.buffer];
            const uint8_t *indexData = gltfBuffer.data.data() + gltfBufferView.byteOffset;

            switch (gltfAccessor.componentType)
            {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                indices.resize(gltfAccessor.count);
                std::memcpy(indices.data(), indexData, gltfBufferView.byteLength);
                break;

            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                indices.resize(gltfAccessor.count);
                for (uint32_t i = 0; i < gltfAccessor.count; ++i)
                {
                    uint16_t index = 0;
                    std::memcpy(&index, indexData + i * sizeof(uint16_t), sizeof(uint16_t));
                    indices[i] = index;
                }
                break;

            default:
                Assert(false);
                break;
            }
        }

        return indices;
    }

    std::vector<VertexAttribute> RetrieveVertexAttributes(const tinygltf::Primitive &gltfPrimitive) const
    {
        std::vector<VertexAttribute> attributes{
            { "POSITION", sizeof(glm::vec3), -1, std::nullopt },
            { "NORMAL", sizeof(glm::vec3), -1, std::nullopt },
            { "TANGENT", sizeof(glm::vec3), -1, std::nullopt },
            { "TEXCOORD_0", sizeof(glm::vec2), -1, std::nullopt }
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
                it->index = accessorIndex;
            }
        }

        return attributes;
    }
};

std::unique_ptr<Scene> SceneLoader::LoadFromFile(const Filepath &path)
{
    const NodeCreator nodeCreator = [](const Scene &scene)
        {
            return new Node(scene);
        };

    const GltfParser parser(path, nodeCreator);

    return parser.CreateScene();
}
