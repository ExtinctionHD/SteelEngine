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
    constexpr glm::vec3 kDefaultBaseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    constexpr glm::vec3 kDefaultSurface = glm::vec3(0.0f, 1.0f, 1.0f);
    constexpr glm::vec3 kDefaultNormal = glm::vec3(0.5f, 0.5f, 1.0f);

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

        return glm::make_vec3(values.data());
    }

    glm::vec4 GetVector4(const std::vector<double> &values)
    {
        Assert(values.size() == glm::vec4::length());

        return glm::make_vec4(values.data());
    }

    glm::quat GetQuaternion(const std::vector<double> &values)
    {
        Assert(values.size() == glm::quat::length());

        return glm::make_quat(values.data());
    }

    vk::Filter GetVkSamplerFilter(int32_t filter)
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

    vk::SamplerMipmapMode GetVkSamplerMipmapMode(int32_t filter)
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

    vk::SamplerAddressMode GetVkSamplerAddressMode(int32_t wrap)
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

    void CalculateTangents(std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices)
    {
        for (size_t i = 0; i < indices.size(); i = i + 3)
        {
            Vertex &vertex0 = vertices[indices[i]];
            Vertex &vertex1 = vertices[indices[i + 1]];
            Vertex &vertex2 = vertices[indices[i + 2]];

            const glm::vec3 edge1 = vertex1.position - vertex0.position;
            const glm::vec3 edge2 = vertex2.position - vertex0.position;

            const glm::vec2 deltaTexCoord1 = vertex1.texCoord - vertex0.texCoord;
            const glm::vec2 deltaTexCoord2 = vertex2.texCoord - vertex0.texCoord;

            const float r = 1.0f / (deltaTexCoord1.x * deltaTexCoord2.y - deltaTexCoord1.y * deltaTexCoord2.x);

            const glm::vec3 tangent(
                    ((edge1.x * deltaTexCoord2.y) - (edge2.x * deltaTexCoord1.y)) * r,
                    ((edge1.y * deltaTexCoord2.y) - (edge2.y * deltaTexCoord1.y)) * r,
                    ((edge1.z * deltaTexCoord2.y) - (edge2.z * deltaTexCoord1.y)) * r);

            vertex0.tangent += tangent;
            vertex1.tangent += tangent;
            vertex2.tangent += tangent;
        }

        for (auto &vertex : vertices)
        {
            vertex.tangent = glm::normalize(vertex.tangent);
        }
    }

    glm::mat4 ApplyParentTransform(const glm::mat4 &localTransform, Node *parent)
    {
        if (parent != nullptr)
        {
            return parent->transform * localTransform;
        }

        return localTransform;
    }
}

using NodeCreator = std::function<Node *(const Scene &)>;

using SceneCreator = std::function<std::unique_ptr<Scene>()>;

class GltfParser
{
public:
    GltfParser(const Filepath &path, const NodeCreator &nodeCreator_)
        : nodeCreator(nodeCreator_)
    {
        directory = path.GetDirectory();
        gltfModel = SSceneLoader::LoadGltfModelFromFile(path);
        Assert(!gltfModel.scenes.empty());
    }

    std::unique_ptr<Scene> CreateScene(const SceneCreator &sceneCreator) const
    {
        std::unique_ptr<Scene> scene = sceneCreator();

        for (const auto &nodeIndex : gltfModel.scenes.front().nodes)
        {
            scene->AddNode(CreateNode(gltfModel.nodes[nodeIndex], *scene, nullptr));
        }

        return scene;
    }

private:
    struct VertexAttribute
    {
        std::string name;
        uint32_t size;
        std::optional<tinygltf::Accessor> gltfAccessor;
    };

    NodeCreator nodeCreator;

    std::string directory;

    tinygltf::Model gltfModel;

    std::unique_ptr<Node> CreateNode(const tinygltf::Node &gltfNode,
            const Scene &scene, Node *parent) const
    {
        Node *node = nodeCreator(scene);
        node->name = gltfNode.name;
        node->transform = SSceneLoader::ApplyParentTransform(FetchTransform(gltfNode), parent);
        node->renderObjects = CreateRenderObjects(gltfNode);

        node->parent = parent;
        node->children.reserve(gltfNode.children.size());

        for (const auto &nodeIndex : gltfNode.children)
        {
            node->children.push_back(CreateNode(gltfModel.nodes[nodeIndex], scene, node));
        }

        return std::unique_ptr<Node>(node);
    }

    glm::mat4 FetchTransform(const tinygltf::Node &gltfNode) const
    {
        if (!gltfNode.matrix.empty())
        {
            return glm::make_mat4(gltfNode.matrix.data());
        }

        glm::mat4 scaleMatrix(1.0f);
        if (!gltfNode.scale.empty())
        {
            const glm::vec3 scale = SSceneLoader::GetVector3(gltfNode.scale);
            scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
        }

        glm::mat4 rotationMatrix(1.0f);
        if (!gltfNode.rotation.empty())
        {
            const glm::quat rotation = SSceneLoader::GetQuaternion(gltfNode.rotation);
            rotationMatrix = glm::toMat4(rotation);
        }

        glm::mat4 translationMatrix(1.0f);
        if (!gltfNode.translation.empty())
        {
            const glm::vec3 translation = SSceneLoader::GetVector3(gltfNode.translation);
            translationMatrix = glm::translate(glm::mat4(1.0f), translation);
        }

        return translationMatrix * rotationMatrix * scaleMatrix;
    }

    std::vector<std::unique_ptr<RenderObject>> CreateRenderObjects(const tinygltf::Node &gltfNode) const
    {
        std::vector<std::unique_ptr<RenderObject>> renderObjects;

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

    std::unique_ptr<RenderObject> CreateRenderObject(const tinygltf::Primitive &gltfPrimitive) const
    {
        std::vector<Vertex> vertices = FetchVertices(gltfPrimitive);
        const std::vector<uint32_t> indices = FetchIndices(gltfPrimitive);

        if (gltfPrimitive.attributes.count("TANGENT") == 0)
        {
            SSceneLoader::CalculateTangents(vertices, indices);
        }

        const Material material = FetchMaterial(gltfPrimitive);

        return std::make_unique<RenderObject>(vertices, indices, material);
    }

    std::vector<Vertex> FetchVertices(const tinygltf::Primitive &gltfPrimitive) const
    {
        const std::vector<VertexAttribute> attributes = FetchVertexAttributes(gltfPrimitive);

        Assert(attributes.front().gltfAccessor.has_value());

        const Vertex defaultVertex{
            glm::vec3(0.0f),
            Direction::kUp,
            glm::vec3(0.0f),
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

    std::vector<uint32_t> FetchIndices(const tinygltf::Primitive &gltfPrimitive) const
    {
        Assert(gltfPrimitive.indices != -1);

        const tinygltf::Accessor &gltfAccessor = gltfModel.accessors[gltfPrimitive.indices];
        const tinygltf::BufferView &gltfBufferView = gltfModel.bufferViews[gltfAccessor.bufferView];
        const tinygltf::Buffer &gltfBuffer = gltfModel.buffers[gltfBufferView.buffer];
        const uint8_t *indexData = gltfBuffer.data.data() + gltfBufferView.byteOffset;

        std::vector<uint32_t> indices;

        switch (gltfAccessor.componentType)
        {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            indices.resize(gltfAccessor.count);
            std::memcpy(indices.data(), indexData, gltfAccessor.count * sizeof(uint32_t));
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

    std::vector<VertexAttribute> FetchVertexAttributes(const tinygltf::Primitive &gltfPrimitive) const
    {
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

        return attributes;
    }

    Material FetchMaterial(const tinygltf::Primitive &gltfPrimitive) const
    {
        const tinygltf::Material &gltfMaterial = gltfModel.materials[gltfPrimitive.material];

        const tinygltf::PbrMetallicRoughness &gltfPbr = gltfMaterial.pbrMetallicRoughness;

        const MaterialFactors factors{
            SSceneLoader::GetVector4(gltfPbr.baseColorFactor),
            static_cast<float>(gltfPbr.roughnessFactor),
            static_cast<float>(gltfPbr.metallicFactor),
            static_cast<float>(gltfMaterial.normalTexture.scale),
        };

        const Material material{
            gltfMaterial.name,
            FetchTexture(gltfPbr.baseColorTexture.index, SSceneLoader::kDefaultBaseColor),
            FetchTexture(gltfPbr.metallicRoughnessTexture.index, SSceneLoader::kDefaultSurface),
            FetchTexture(gltfMaterial.normalTexture.index, SSceneLoader::kDefaultNormal),
            factors
        };

        return material;
    }

    Texture FetchTexture(int32_t textureIndex, const glm::vec3 &defaultColor) const
    {
        if (textureIndex == -1)
        {
            return VulkanContext::textureManager->CreateColorTexture(defaultColor);
        }

        const tinygltf::Texture &gltfTexture = gltfModel.textures[textureIndex];
        const tinygltf::Image &gltfImage = gltfModel.images[gltfTexture.source];

        Assert(!gltfImage.uri.empty());
        const Filepath texturePath(directory + gltfImage.uri);

        Texture texture = VulkanContext::textureManager->CreateTexture(texturePath);

        if (gltfTexture.sampler != -1)
        {
            const tinygltf::Sampler gltfSampler = gltfModel.samplers[gltfTexture.sampler];

            Assert(gltfSampler.wrapS == gltfSampler.wrapR);
            const SamplerDescription samplerDescription{
                SSceneLoader::GetVkSamplerFilter(gltfSampler.magFilter),
                SSceneLoader::GetVkSamplerFilter(gltfSampler.minFilter),
                SSceneLoader::GetVkSamplerMipmapMode(gltfSampler.magFilter),
                SSceneLoader::GetVkSamplerAddressMode(gltfSampler.wrapS),
                VulkanConfig::kMaxAnisotropy,
                0.0f, std::numeric_limits<float>::max()
            };

            texture.sampler = VulkanContext::textureManager->CreateSampler(samplerDescription);
        }

        return texture;
    }
};

std::unique_ptr<Scene> SceneLoader::LoadFromFile(const Filepath &path)
{
    const NodeCreator nodeCreator = [](const Scene &scene)
        {
            return new Node(scene);
        };

    const SceneCreator sceneCreator = []()
        {
            return std::unique_ptr<Scene>(new Scene());
        };

    const GltfParser parser(path, nodeCreator);

    return parser.CreateScene(sceneCreator);
}
