#pragma warning(push, 0)
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#include <tiny_gltf.h>
#pragma warning(pop)

#include "Engine/Scene/SceneLoader.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"

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

    glm::vec3 GetVector3(const std::vector<double> &values)
    {
        Assert(values.size() == glm::vec3::length());

        return glm::vec3(static_cast<float>(values[0]), static_cast<float>(values[1]), static_cast<float>(values[2]));
    }

    glm::vec4 GetVector4(const std::vector<double> &values)
    {
        Assert(values.size() == glm::vec4::length());

        return glm::vec4(static_cast<float>(values[0]), static_cast<float>(values[1]),
                static_cast<float>(values[2]), static_cast<float>(values[3]));
    }

    glm::quat GetQuaternion(const std::vector<double> &values)
    {
        Assert(values.size() == glm::quat::length());

        return glm::quat(static_cast<float>(values[0]), static_cast<float>(values[1]),
                static_cast<float>(values[2]), static_cast<float>(values[3]));
    }

    vk::Filter GetVkSamplerFilter(int filter)
    {
        switch (filter)
        {
        case 0:
        case 2:
        case 4:
            return vk::Filter::eNearest;

        case 3:
        case 5:
            return vk::Filter::eLinear;

        default:
            return vk::Filter::eLinear;
        }
    }

    vk::SamplerMipmapMode GetVkSamplerMipmapMode(int filter)
    {
        switch (filter)
        {
        case 0:
        case 1:
        case 2:
        case 4:
        case 5:
            return vk::SamplerMipmapMode::eLinear;

        case 3:
            return vk::SamplerMipmapMode::eNearest;

        default:
            return vk::SamplerMipmapMode::eLinear;
        }
    }

    vk::SamplerAddressMode GetVkSamplerAddressMode(int wrap)
    {
        switch (wrap)
        {
        case 0:
            return vk::SamplerAddressMode::eClampToEdge;
        case 1:
            return vk::SamplerAddressMode::eMirroredRepeat;
        case 2:
            return vk::SamplerAddressMode::eRepeat;

        default:
            return vk::SamplerAddressMode::eRepeat;
        }
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

        directory = path.GetDirectory();
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

    std::string directory;

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
            const glm::vec3 translation = SSceneLoader::GetVector3(gltfNode.translation);

            transform = glm::translate(transform, translation);
        }

        if (!gltfNode.rotation.empty())
        {
            glm::quat rotation = SSceneLoader::GetQuaternion(gltfNode.rotation);

            rotation = glm::rotate(rotation, glm::pi<float>(), Direction::kRight);

            transform = glm::toMat4(rotation) * transform;
        }

        if (!gltfNode.scale.empty())
        {
            const glm::vec3 scale = SSceneLoader::GetVector3(gltfNode.scale);

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
        const Material material = RetrieveMaterial(gltfPrimitive);

        return RenderObject(vertices, indices, material);
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
        if (gltfPrimitive.indices == -1)
        {
            return {};
        }

        const tinygltf::Accessor &gltfAccessor = gltfModel.accessors[gltfPrimitive.indices];
        const tinygltf::BufferView &gltfBufferView = gltfModel.bufferViews[gltfAccessor.bufferView];
        const tinygltf::Buffer &gltfBuffer = gltfModel.buffers[gltfBufferView.buffer];
        const uint8_t *indexData = gltfBuffer.data.data() + gltfBufferView.byteOffset;

        std::vector<uint32_t> indices;

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

    Material RetrieveMaterial(const tinygltf::Primitive &gltfPrimitive) const
    {
        const tinygltf::Material &gltfMaterial = gltfModel.materials[gltfPrimitive.material];

        const tinygltf::PbrMetallicRoughness &gltfPbr = gltfMaterial.pbrMetallicRoughness;

        const Material material{
            gltfMaterial.name,
            RetrieveTexture(gltfPbr.baseColorTexture.index),
            RetrieveTexture(gltfPbr.metallicRoughnessTexture.index),
            RetrieveTexture(gltfMaterial.occlusionTexture.index),
            RetrieveTexture(gltfMaterial.normalTexture.index),
            SSceneLoader::GetVector4(gltfPbr.baseColorFactor),
            static_cast<float>(gltfPbr.metallicFactor),
            static_cast<float>(gltfPbr.roughnessFactor),
            static_cast<float>(gltfMaterial.occlusionTexture.strength),
            static_cast<float>(gltfMaterial.normalTexture.scale)
        };

        return material;
    }

    Texture RetrieveTexture(int textureIndex) const
    {
        if (textureIndex == -1)
        {
            Assert(false); // TODO

            return Texture{};
        }

        const tinygltf::Texture &gltfTexture = gltfModel.textures[textureIndex];
        const tinygltf::Image &gltfImage = gltfModel.images[gltfTexture.source];
        const tinygltf::Sampler gltfSampler = gltfModel.samplers[gltfTexture.sampler];

        Assert(!gltfImage.uri.empty());
        Assert(gltfSampler.wrapS == gltfSampler.wrapR);

        const Filepath texturePath(directory + gltfImage.uri);

        const SamplerDescription samplerDescription{
            SSceneLoader::GetVkSamplerFilter(gltfSampler.magFilter),
            SSceneLoader::GetVkSamplerFilter(gltfSampler.minFilter),
            SSceneLoader::GetVkSamplerMipmapMode(gltfSampler.magFilter),
            SSceneLoader::GetVkSamplerAddressMode(gltfSampler.wrapS),
            VulkanConfig::kMaxAnisotropy,
            0.0f, std::numeric_limits<float>::max()
        };

        return VulkanContext::textureCache->GetTexture(texturePath, samplerDescription);
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
