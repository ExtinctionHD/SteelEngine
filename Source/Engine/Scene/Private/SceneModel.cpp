#pragma warning(push, 0)
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#include <tiny_gltf.h>
#pragma warning(pop)

#include "Engine/Scene/SceneModel.hpp"

#include "Engine/Camera.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/SceneRT.hpp"
#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Renderer.hpp"
#include "Engine/EngineHelpers.hpp"
#include "Engine/Config.hpp"

#include "Shaders/Forward//Forward.h"
#include "Shaders/RayTracing/RayTracing.h"

#include "Utils/Assert.hpp"
#include "Utils/TimeHelpers.hpp"

namespace Helpers
{
    static vk::Format GetFormat(const tinygltf::Image& image)
    {
        Assert(image.bits == 8);
        Assert(image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE);

        switch (image.component)
        {
        case 1:
            return vk::Format::eR8Unorm;
        case 2:
            return vk::Format::eR8G8Unorm;
        case 3:
            return vk::Format::eR8G8B8Unorm;
        case 4:
            return vk::Format::eR8G8B8A8Unorm;
        default:
            return vk::Format::eUndefined;
        }
    }

    static vk::Filter GetSamplerFilter(int32_t filter)
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

    static vk::SamplerMipmapMode GetSamplerMipmapMode(int32_t filter)
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

    static vk::SamplerAddressMode GetSamplerAddressMode(int32_t wrap)
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

    static vk::IndexType GetIndexType(int32_t type)
    {
        switch (type)
        {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return vk::IndexType::eUint16;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            return vk::IndexType::eUint32;
        default:
            return vk::IndexType::eNoneKHR;
        }
    }

    template <glm::length_t L>
    static glm::vec<L, float, glm::defaultp> GetVec(const std::vector<double>& values)
    {
        const glm::length_t valueCount = static_cast<glm::length_t>(values.size());

        glm::vec<L, float, glm::defaultp> result(0.0f);

        for (glm::length_t i = 0; i < valueCount && i < L; ++i)
        {
            result[i] = static_cast<float>(values[i]);
        }

        return result;
    }

    static glm::quat GetQuaternion(const std::vector<double>& values)
    {
        Assert(values.size() == glm::quat::length());

        return glm::make_quat(values.data());
    }

    static glm::mat4 GetTransform(const tinygltf::Node& node)
    {
        if (!node.matrix.empty())
        {
            return glm::make_mat4(node.matrix.data());
        }

        glm::mat4 scaleMatrix(1.0f);
        if (!node.scale.empty())
        {
            const glm::vec3 scale = GetVec<3>(node.scale);
            scaleMatrix = glm::scale(Matrix4::kIdentity, scale);
        }

        glm::mat4 rotationMatrix(1.0f);
        if (!node.rotation.empty())
        {
            const glm::quat rotation = GetQuaternion(node.rotation);
            rotationMatrix = glm::toMat4(rotation);
        }

        glm::mat4 translationMatrix(1.0f);
        if (!node.translation.empty())
        {
            const glm::vec3 translation = GetVec<3>(node.translation);
            translationMatrix = glm::translate(Matrix4::kIdentity, translation);
        }

        return translationMatrix * rotationMatrix * scaleMatrix;
    }

    static size_t GetAccessorValueSize(const tinygltf::Accessor& accessor)
    {
        const int32_t count = tinygltf::GetNumComponentsInType(accessor.type);
        Assert(count >= 0);

        const int32_t size = tinygltf::GetComponentSizeInBytes(accessor.componentType);
        Assert(size >= 0);

        return static_cast<size_t>(count) * static_cast<size_t>(size);
    }

    template <class T>
    static DataView<T> GetAccessorDataView(const tinygltf::Model& model,
            const tinygltf::Accessor& accessor)
    {
        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
        Assert(bufferView.byteStride == 0 || bufferView.byteStride == GetAccessorValueSize(accessor));

        const size_t offset = bufferView.byteOffset + accessor.byteOffset;
        const T* data = reinterpret_cast<const T*>(model.buffers[bufferView.buffer].data.data() + offset);

        return DataView<T>(data, accessor.count);
    }

    static ByteView GetAccessorByteView(const tinygltf::Model& model,
            const tinygltf::Accessor& accessor)
    {
        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
        Assert(bufferView.byteStride == 0);

        const size_t offset = bufferView.byteOffset + accessor.byteOffset;
        const uint8_t* data = model.buffers[bufferView.buffer].data.data() + offset;

        return DataView<uint8_t>(data, bufferView.byteLength - accessor.byteOffset);
    }

    template <class T>
    static T GetAccessorValue(const tinygltf::Model& model,
            const tinygltf::Accessor& accessor, size_t index)
    {
        const size_t size = GetAccessorValueSize(accessor);

        Assert(sizeof(T) <= size);

        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];

        const size_t offset = bufferView.byteOffset + accessor.byteOffset;
        const size_t stride = bufferView.byteStride != 0 ? bufferView.byteStride : size;

        const uint8_t* data = model.buffers[bufferView.buffer].data.data();

        return *reinterpret_cast<const T*>(data + offset + stride * index);
    }
}

namespace Details
{
    using NodeFunctor = std::function<void(int32_t, const glm::mat4&)>;

    static void CalculateNormals(const std::vector<uint32_t>& indices,
            std::vector<Scene::Mesh::Vertex>& vertices)
    {
        for (auto& vertex : vertices)
        {
            vertex.normal = glm::vec3();
        }

        for (size_t i = 0; i < indices.size(); i = i + 3)
        {
            const glm::vec3& position0 = vertices[indices[i]].position;
            const glm::vec3& position1 = vertices[indices[i + 1]].position;
            const glm::vec3& position2 = vertices[indices[i + 2]].position;

            const glm::vec3 edge1 = position1 - position0;
            const glm::vec3 edge2 = position2 - position0;

            const glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

            vertices[indices[i]].normal += normal;
            vertices[indices[i + 1]].normal += normal;
            vertices[indices[i + 2]].normal += normal;
        }

        for (auto& vertex : vertices)
        {
            vertex.normal = glm::normalize(vertex.normal);
        }
    }

    static void CalculateTangents(const std::vector<uint32_t>& indices,
            std::vector<Scene::Mesh::Vertex>& vertices)
    {
        for (auto& vertex : vertices)
        {
            vertex.tangent = glm::vec3();
        }

        for (size_t i = 0; i < indices.size(); i = i + 3)
        {
            const glm::vec3& position0 = vertices[indices[i]].position;
            const glm::vec3& position1 = vertices[indices[i + 1]].position;
            const glm::vec3& position2 = vertices[indices[i + 2]].position;

            const glm::vec3 edge1 = position1 - position0;
            const glm::vec3 edge2 = position2 - position0;

            const glm::vec2& texCoord0 = vertices[indices[i]].texCoord;
            const glm::vec2& texCoord1 = vertices[indices[i + 1]].texCoord;
            const glm::vec2& texCoord2 = vertices[indices[i + 2]].texCoord;

            const glm::vec2 deltaTexCoord1 = texCoord1 - texCoord0;
            const glm::vec2 deltaTexCoord2 = texCoord2 - texCoord0;

            float d = deltaTexCoord1.x * deltaTexCoord2.y - deltaTexCoord1.y * deltaTexCoord2.x;

            if (d == 0.0f)
            {
                d = 1.0f;
            }

            const glm::vec3 tangent = (edge1 * deltaTexCoord2.y - edge2 * deltaTexCoord1.y) / d;

            vertices[indices[i]].tangent += tangent;
            vertices[indices[i + 1]].tangent += tangent;
            vertices[indices[i + 2]].tangent += tangent;
        }

        for (auto& vertex : vertices)
        {
            if (glm::length(vertex.tangent) > 0.0f)
            {
                vertex.tangent = glm::normalize(vertex.tangent);
            }
            else
            {
                vertex.tangent.x = 1.0f;
            }
        }
    }

    static uint32_t CalculateMeshOffset(const tinygltf::Model& model, uint32_t meshIndex)
    {
        uint32_t offset = 0;

        for (size_t i = 0; i < meshIndex; ++i)
        {
            offset += static_cast<uint32_t>(model.meshes[i].primitives.size());
        }

        return offset;
    }

    static void EnumerateNodes(const tinygltf::Model& model, const NodeFunctor& functor)
    {
        const NodeFunctor enumerator = [&](int32_t nodeIndex, const glm::mat4& parentTransform)
            {
                const tinygltf::Node& node = model.nodes[nodeIndex];
                const glm::mat4 transform = parentTransform * Helpers::GetTransform(node);

                for (const auto& childIndex : node.children)
                {
                    enumerator(childIndex, transform);
                }

                functor(nodeIndex, transform);
            };

        for (const auto& scene : model.scenes)
        {
            for (const auto& nodeIndex : scene.nodes)
            {
                enumerator(nodeIndex, Matrix4::kIdentity);
            }
        }
    }

    static std::vector<uint32_t> GetPrimitiveIndices(const tinygltf::Model& model,
            const tinygltf::Primitive& primitive)
    {
        const tinygltf::Accessor& accessor = model.accessors[primitive.indices];

        std::vector<uint32_t> indices(accessor.count);
        for (size_t i = 0; i < indices.size(); ++i)
        {
            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
            {
                indices[i] = Helpers::GetAccessorValue<uint32_t>(model, accessor, i);
            }
            else
            {
                Assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);

                indices[i] = static_cast<uint32_t>(Helpers::GetAccessorValue<uint16_t>(model, accessor, i));
            }
        }

        return indices;
    }

    static std::vector<Scene::Mesh::Vertex> GetPrimitiveVertices(const tinygltf::Model& model,
            const tinygltf::Primitive& primitive)
    {
        const tinygltf::Accessor& positionsAccessor = model.accessors[primitive.attributes.at("POSITION")];
        const tinygltf::Accessor& texCoordsAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];

        std::vector<Scene::Mesh::Vertex> vertices(positionsAccessor.count);
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            Scene::Mesh::Vertex& vertex = vertices[i];

            vertex.position = Helpers::GetAccessorValue<glm::vec3>(model, positionsAccessor, i);

            if (primitive.attributes.count("NORMAL") > 0)
            {
                const tinygltf::Accessor& normalsAccessor = model.accessors[primitive.attributes.at("NORMAL")];
                vertex.normal = Helpers::GetAccessorValue<glm::vec3>(model, normalsAccessor, i);
            }

            if (primitive.attributes.count("TANGENT") > 0)
            {
                const tinygltf::Accessor& tangentsAccessor = model.accessors[primitive.attributes.at("TANGENT")];
                vertex.tangent = Helpers::GetAccessorValue<glm::vec3>(model, tangentsAccessor, i);
            }

            vertex.texCoord = Helpers::GetAccessorValue<glm::vec2>(model, texCoordsAccessor, i);
        }

        return vertices;
    }

    static std::vector<Texture> CreateTextures(const tinygltf::Model& model)
    {
        std::vector<Texture> textures;
        textures.reserve(model.images.size());

        for (const auto& image : model.images)
        {
            const vk::Format format = Helpers::GetFormat(image);
            const vk::Extent2D extent = VulkanHelpers::GetExtent(image.width, image.height);

            textures.push_back(VulkanContext::textureManager->CreateTexture(format, extent, ByteView(image.image)));
        }

        return textures;
    }

    static std::vector<vk::Sampler> CreateSamplers(const tinygltf::Model& model)
    {
        std::vector<vk::Sampler> samplers;
        samplers.reserve(model.samplers.size());

        for (const auto& sampler : model.samplers)
        {
            Assert(sampler.wrapS == sampler.wrapT);

            const SamplerDescription samplerDescription{
                Helpers::GetSamplerFilter(sampler.magFilter),
                Helpers::GetSamplerFilter(sampler.minFilter),
                Helpers::GetSamplerMipmapMode(sampler.magFilter),
                Helpers::GetSamplerAddressMode(sampler.wrapS),
                VulkanConfig::kMaxAnisotropy,
                0.0f, std::numeric_limits<float>::max()
            };

            samplers.push_back(VulkanContext::textureManager->CreateSampler(samplerDescription));
        }

        return samplers;
    }

    static std::vector<Scene::Mesh> CreateMeshes(const tinygltf::Model& model)
    {
        std::vector<Scene::Mesh> meshes;

        for (const auto& mesh : model.meshes)
        {
            for (const auto& primitive : mesh.primitives)
            {
                Assert(primitive.mode == TINYGLTF_MODE_TRIANGLES);
                Assert(primitive.indices >= 0);

                const std::vector<uint32_t> indices = GetPrimitiveIndices(model, primitive);

                std::vector<Scene::Mesh::Vertex> vertices = GetPrimitiveVertices(model, primitive);

                if (primitive.attributes.count("NORMAL") == 0)
                {
                    CalculateNormals(indices, vertices);
                }
                if (primitive.attributes.count("TANGENT") == 0)
                {
                    CalculateTangents(indices, vertices);
                }

                const vk::Buffer indexBuffer = BufferHelpers::CreateDeviceLocalBufferWithData(
                        vk::BufferUsageFlagBits::eIndexBuffer, ByteView(indices), SyncScope::kIndicesRead);

                const vk::Buffer vertexBuffer = BufferHelpers::CreateDeviceLocalBufferWithData(
                        vk::BufferUsageFlagBits::eVertexBuffer, ByteView(vertices), SyncScope::kVerticesRead);

                meshes.push_back(Scene::Mesh{
                    vk::IndexType::eUint32, indexBuffer, static_cast<uint32_t>(indices.size()),
                    vertexBuffer, static_cast<uint32_t>(vertices.size())
                });
            }
        }

        return meshes;
    }

    static std::vector<Scene::Material> CreateMaterials(const tinygltf::Model& model)
    {
        std::vector<Scene::Material> materials;

        for (const auto& material : model.materials)
        {
            Assert(material.pbrMetallicRoughness.baseColorTexture.texCoord == 0);
            Assert(material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord == 0);
            Assert(material.normalTexture.texCoord == 0);
            Assert(material.occlusionTexture.texCoord == 0);
            Assert(material.emissiveTexture.texCoord == 0);

            const Material shaderMaterial{
                Helpers::GetVec<4>(material.pbrMetallicRoughness.baseColorFactor),
                Helpers::GetVec<4>(material.emissiveFactor),
                static_cast<float>(material.pbrMetallicRoughness.roughnessFactor),
                static_cast<float>(material.pbrMetallicRoughness.metallicFactor),
                static_cast<float>(material.normalTexture.scale),
                static_cast<float>(material.occlusionTexture.strength),
            };

            const bool alphaTested = material.alphaMode != "OPAQUE";

            const glm::vec4 alphaCutoff(static_cast<float>(material.alphaCutoff), 0.0f, 0.0f, 0.0f);

            const Bytes shaderData = alphaTested ? GetBytes(shaderMaterial, alphaCutoff) : GetBytes(shaderMaterial);

            const vk::Buffer materialBuffer = BufferHelpers::CreateDeviceLocalBufferWithData(
                    vk::BufferUsageFlagBits::eUniformBuffer, ByteView(shaderData), SyncScope::kFragmentShaderRead);

            const Scene::Material sceneMaterial{
                Scene::PipelineState{ alphaTested, material.doubleSided },
                material.pbrMetallicRoughness.baseColorTexture.index,
                material.pbrMetallicRoughness.metallicRoughnessTexture.index,
                material.normalTexture.index,
                material.occlusionTexture.index,
                material.emissiveTexture.index,
                materialBuffer
            };

            materials.push_back(sceneMaterial);
        }

        return materials;
    }

    static std::vector<Scene::RenderObject> CreateRenderObjects(const tinygltf::Model& model)
    {
        std::vector<Scene::RenderObject> renderObjects;

        Details::EnumerateNodes(model, [&](int32_t nodeIndex, const glm::mat4& transform)
            {
                const tinygltf::Node& node = model.nodes[nodeIndex];

                if (node.mesh >= 0)
                {
                    const tinygltf::Mesh& mesh = model.meshes[node.mesh];

                    for (uint32_t i = 0; i < static_cast<uint32_t>(mesh.primitives.size()); ++i)
                    {
                        const uint32_t meshIndex = CalculateMeshOffset(model, node.mesh) + i;
                        const uint32_t materialIndex = static_cast<const uint32_t>(mesh.primitives[i].material);

                        const Scene::RenderObject renderObject{
                            meshIndex,
                            materialIndex,
                            transform
                        };

                        renderObjects.push_back(renderObject);
                    }
                }
            });

        return renderObjects;
    }

    static std::vector<vk::Buffer> CollectBuffers(const Scene::Hierarchy& sceneHierarchy)
    {
        std::vector<vk::Buffer> buffers;

        for (const auto& mesh : sceneHierarchy.meshes)
        {
            buffers.push_back(mesh.indexBuffer);
            buffers.push_back(mesh.vertexBuffer);
        }

        for (const auto& material : sceneHierarchy.materials)
        {
            buffers.push_back(material.buffer);
        }

        return buffers;
    }

    static MultiDescriptorSet CreateMaterialsDescriptorSet(const tinygltf::Model& model,
            const Scene::Hierarchy& hierarchy, const Scene::Resources& resources)
    {
        const DescriptorSetDescription descriptorSetDescription{
            DescriptorDescription{
                1, vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eFragment,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                1, vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eFragment,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                1, vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eFragment,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                1, vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eFragment,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                1, vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eFragment,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                1, vk::DescriptorType::eUniformBuffer,
                vk::ShaderStageFlagBits::eFragment,
                vk::DescriptorBindingFlags()
            }
        };

        std::vector<DescriptorSetData> multiDescriptorSetData;
        multiDescriptorSetData.reserve(hierarchy.materials.size());


        std::array<Texture, Scene::Material::kTextureCount> placeholders{
            Renderer::whiteTexture,
            Renderer::whiteTexture,
            Renderer::normalTexture,
            Renderer::whiteTexture,
            Renderer::whiteTexture
        };

        std::array<std::optional<tinygltf::Texture>, Scene::Material::kTextureCount> textures;

        for (const auto& material : hierarchy.materials)
        {
            if (material.baseColorTexture >= 0)
            {
                textures[0] = model.textures[material.baseColorTexture];
            }
            if (material.roughnessMetallicTexture >= 0)
            {
                textures[1] = model.textures[material.roughnessMetallicTexture];
            }
            if (material.normalTexture >= 0)
            {
                textures[2] = model.textures[material.normalTexture];
            }
            if (material.occlusionTexture >= 0)
            {
                textures[3] = model.textures[material.occlusionTexture];
            }
            if (material.emissionTexture >= 0)
            {
                textures[4] = model.textures[material.emissionTexture];
            }

            DescriptorSetData descriptorSetData(Scene::Material::kTextureCount + 1);
            for (uint32_t i = 0; i < Scene::Material::kTextureCount; ++i)
            {
                vk::Sampler sampler = Renderer::defaultSampler;
                vk::ImageView view = placeholders[i].view;

                if (textures[i].has_value())
                {
                    if (textures[i]->sampler >= 0)
                    {
                        sampler = resources.samplers[textures[i]->sampler];
                    }
                    if (textures[i]->source >= 0)
                    {
                        view = resources.textures[textures[i]->source].view;
                    }
                }

                descriptorSetData[i] = DescriptorHelpers::GetData(sampler, view);
            }

            descriptorSetData.back() = DescriptorHelpers::GetData(material.buffer);

            multiDescriptorSetData.push_back(descriptorSetData);
        }

        return DescriptorHelpers::CreateMultiDescriptorSet(descriptorSetDescription, multiDescriptorSetData);
    }
}

namespace DetailsRT
{
    struct AccelerationData
    {
        vk::AccelerationStructureKHR tlas;
        std::vector<vk::AccelerationStructureKHR> blases;
    };

    struct GeometryData
    {
        SceneRT::DescriptorSets descriptorSets;
        std::vector<vk::Buffer> buffers;
    };

    struct TexturesData
    {
        DescriptorSet descriptorSet;
        std::vector<Texture> textures;
        std::vector<vk::Sampler> samplers;
    };

    struct MaterialsData
    {
        vk::Buffer buffer;
    };

    using AccelerationStructures = std::vector<vk::AccelerationStructureKHR>;

    using GeometryBuffers = std::map<SceneRT::DescriptorSetType, BufferInfo>;

    static uint32_t GetCustomIndex(uint16_t instanceIndex, uint8_t materialIndex)
    {
        return static_cast<uint32_t>(instanceIndex) | (static_cast<uint32_t>(materialIndex) << 16);
    }

    static GeometryVertexData CreateGeometryPositions(const tinygltf::Model& model,
            const tinygltf::Primitive& primitive)
    {
        Assert(primitive.mode == TINYGLTF_MODE_TRIANGLES);

        const tinygltf::Accessor accessor = model.accessors[primitive.attributes.at("POSITION")];
        const DataView<glm::vec3> data = Helpers::GetAccessorDataView<glm::vec3>(model, accessor);

        const vk::BufferUsageFlags bufferUsage = vk::BufferUsageFlagBits::eShaderDeviceAddressEXT;

        const vk::Buffer buffer = BufferHelpers::CreateDeviceLocalBufferWithData(
                bufferUsage, ByteView(data), SyncScope::kAccelerationStructureBuild);

        const GeometryVertexData vertices{
            buffer,
            vk::Format::eR32G32B32Sfloat,
            static_cast<uint32_t>(accessor.count),
            sizeof(glm::vec3)
        };

        return vertices;
    }

    static GeometryIndexData CreateGeometryIndices(const tinygltf::Model& model,
            const tinygltf::Primitive& primitive)
    {
        Assert(primitive.indices >= 0);

        const tinygltf::Accessor accessor = model.accessors[primitive.indices];
        const ByteView data = Helpers::GetAccessorByteView(model, accessor);

        const vk::BufferUsageFlags bufferUsage = vk::BufferUsageFlagBits::eShaderDeviceAddressEXT;

        const SyncScope blockScope = SyncScope::kAccelerationStructureBuild;
        const vk::Buffer buffer = BufferHelpers::CreateDeviceLocalBufferWithData(bufferUsage, data, blockScope);

        const GeometryIndexData indices{
            buffer,
            Helpers::GetIndexType(accessor.componentType),
            static_cast<uint32_t>(accessor.count)
        };

        return indices;
    }

    static AccelerationStructures GenerateBlases(const tinygltf::Model& model)
    {
        std::vector<vk::AccelerationStructureKHR> blases;
        blases.reserve(model.meshes.size());

        for (const auto& mesh : model.meshes)
        {
            for (const auto& primitive : mesh.primitives)
            {
                const GeometryVertexData vertices = CreateGeometryPositions(model, primitive);
                const GeometryIndexData indices = CreateGeometryIndices(model, primitive);

                blases.push_back(VulkanContext::accelerationStructureManager->GenerateBlas(vertices, indices));

                VulkanContext::bufferManager->DestroyBuffer(vertices.buffer);
                VulkanContext::bufferManager->DestroyBuffer(indices.buffer);
            }
        }

        return blases;
    }

    static AccelerationData CreateAccelerationData(const tinygltf::Model& model)
    {
        const std::vector<vk::AccelerationStructureKHR> blases = GenerateBlases(model);

        std::vector<GeometryInstanceData> instances;

        Details::EnumerateNodes(model, [&](int32_t nodeIndex, const glm::mat4& transform)
            {
                const tinygltf::Node& node = model.nodes[nodeIndex];

                if (node.mesh >= 0)
                {
                    const tinygltf::Mesh& mesh = model.meshes[node.mesh];

                    for (size_t i = 0; i < mesh.primitives.size(); ++i)
                    {
                        const vk::AccelerationStructureKHR blas = blases[Details::CalculateMeshOffset(model, node.mesh)
                            + i];

                        const uint16_t instanceIndex = static_cast<uint16_t>(instances.size());
                        const uint8_t materialIndex = static_cast<uint8_t>(mesh.primitives[i].material);

                        const vk::GeometryInstanceFlagsKHR flags = vk::GeometryInstanceFlagBitsKHR::eForceOpaque;

                        const GeometryInstanceData instance{
                            blas, transform,
                            GetCustomIndex(instanceIndex, materialIndex),
                            0xFF, 0, flags
                        };

                        instances.push_back(instance);
                    }
                }
            });

        const vk::AccelerationStructureKHR tlas = VulkanContext::accelerationStructureManager->GenerateTlas(instances);

        return AccelerationData{ tlas, blases };
    }

    static std::vector<glm::vec3> CalculateNormals(const DataView<uint32_t>& indices,
            const DataView<glm::vec3>& positions)
    {
        std::vector<glm::vec3> normals(positions.size);

        for (size_t i = 0; i < indices.size; i = i + 3)
        {
            const glm::vec3& position0 = positions.data[indices.data[i]];
            const glm::vec3& position1 = positions.data[indices.data[i + 1]];
            const glm::vec3& position2 = positions.data[indices.data[i + 2]];

            const glm::vec3 edge1 = position1 - position0;
            const glm::vec3 edge2 = position2 - position0;

            const glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

            normals[indices.data[i]] += normal;
            normals[indices.data[i + 1]] += normal;
            normals[indices.data[i + 2]] += normal;
        }

        for (auto& normal : normals)
        {
            normal = glm::normalize(normal);
        }

        return normals;
    }

    static std::vector<glm::vec3> CalculateTangents(const DataView<uint32_t>& indices,
            const DataView<glm::vec3>& positions, const DataView<glm::vec2>& texCoords)
    {
        std::vector<glm::vec3> tangents(positions.size);

        for (size_t i = 0; i < indices.size; i = i + 3)
        {
            const glm::vec3& position0 = positions.data[indices.data[i]];
            const glm::vec3& position1 = positions.data[indices.data[i + 1]];
            const glm::vec3& position2 = positions.data[indices.data[i + 2]];

            const glm::vec3 edge1 = position1 - position0;
            const glm::vec3 edge2 = position2 - position0;

            const glm::vec2& texCoord0 = texCoords.data[indices.data[i]];
            const glm::vec2& texCoord1 = texCoords.data[indices.data[i + 1]];
            const glm::vec2& texCoord2 = texCoords.data[indices.data[i + 2]];

            const glm::vec2 deltaTexCoord1 = texCoord1 - texCoord0;
            const glm::vec2 deltaTexCoord2 = texCoord2 - texCoord0;

            float d = deltaTexCoord1.x * deltaTexCoord2.y - deltaTexCoord1.y * deltaTexCoord2.x;

            if (d == 0.0f)
            {
                d = 1.0f;
            }

            const glm::vec3 tangent = (edge1 * deltaTexCoord2.y - edge2 * deltaTexCoord1.y) / d;

            tangents[indices.data[i]] += tangent;
            tangents[indices.data[i + 1]] += tangent;
            tangents[indices.data[i + 2]] += tangent;
        }

        for (auto& tangent : tangents)
        {
            if (glm::length(tangent) > 0.0f)
            {
                tangent = glm::normalize(tangent);
            }
            else
            {
                tangent.x = 1.0f;
            }
        }

        return tangents;
    }

    static void AppendPrimitiveGeometryBuffers(const tinygltf::Model& model,
            const tinygltf::Primitive& primitive, GeometryBuffers& buffers)
    {
        Assert(primitive.mode == TINYGLTF_MODE_TRIANGLES);
        Assert(primitive.indices >= 0);

        const tinygltf::Accessor& indicesAccessor = model.accessors[primitive.indices];

        DataView<uint32_t> indicesData = Helpers::GetAccessorDataView<uint32_t>(model, indicesAccessor);
        std::vector<uint32_t> indices;
        if (indicesAccessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
        {
            Assert(indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);

            indices.resize(indicesAccessor.count);

            for (size_t i = 0; i < indicesAccessor.count; ++i)
            {
                indices[i] = reinterpret_cast<const uint16_t*>(indicesData.data)[i];
            }

            indicesData = DataView(indices);
        }

        const tinygltf::Accessor& positionsAccessor = model.accessors[primitive.attributes.at("POSITION")];
        const DataView<glm::vec3> positionsData = Helpers::GetAccessorDataView<glm::vec3>(model, positionsAccessor);

        const tinygltf::Accessor& texCoordsAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
        const DataView<glm::vec2> texCoordsData = Helpers::GetAccessorDataView<glm::vec2>(model, texCoordsAccessor);

        std::vector<glm::vec3> normals;
        DataView<glm::vec3> normalsData;
        if (primitive.attributes.count("NORMAL") > 0)
        {
            const tinygltf::Accessor& normalsAccessor = model.accessors[primitive.attributes.at("NORMAL")];
            normalsData = Helpers::GetAccessorDataView<glm::vec3>(model, normalsAccessor);
        }
        else
        {
            normals = CalculateNormals(indicesData, positionsData);
            normalsData = DataView(normals);
        }

        std::vector<glm::vec3> tangents;
        DataView<glm::vec3> tangentsData;
        if (primitive.attributes.count("TANGENT") > 0)
        {
            const tinygltf::Accessor& tangentsAccessor = model.accessors[primitive.attributes.at("NORMAL")];
            tangentsData = Helpers::GetAccessorDataView<glm::vec3>(model, tangentsAccessor);
        }
        else
        {
            tangents = CalculateTangents(indicesData, positionsData, texCoordsData);
            tangentsData = DataView(tangents);
        }

        const vk::BufferUsageFlags bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer;

        const vk::Buffer indicesBuffer = BufferHelpers::CreateDeviceLocalBufferWithData(
                bufferUsage, ByteView(indicesData), SyncScope::kRayTracingShaderRead);
        const vk::Buffer positionsBuffer = BufferHelpers::CreateDeviceLocalBufferWithData(
                bufferUsage, ByteView(positionsData), SyncScope::kRayTracingShaderRead);
        const vk::Buffer normalsBuffer = BufferHelpers::CreateDeviceLocalBufferWithData(
                bufferUsage, ByteView(normalsData), SyncScope::kRayTracingShaderRead);
        const vk::Buffer tangentsBuffer = BufferHelpers::CreateDeviceLocalBufferWithData(
                bufferUsage, ByteView(tangentsData), SyncScope::kRayTracingShaderRead);
        const vk::Buffer texCoordsBuffer = BufferHelpers::CreateDeviceLocalBufferWithData(
                bufferUsage, ByteView(texCoordsData), SyncScope::kRayTracingShaderRead);

        buffers[SceneRT::DescriptorSetType::eIndices].emplace_back(indicesBuffer, 0, VK_WHOLE_SIZE);
        buffers[SceneRT::DescriptorSetType::ePositions].emplace_back(positionsBuffer, 0, VK_WHOLE_SIZE);
        buffers[SceneRT::DescriptorSetType::eNormals].emplace_back(normalsBuffer, 0, VK_WHOLE_SIZE);
        buffers[SceneRT::DescriptorSetType::eTangents].emplace_back(tangentsBuffer, 0, VK_WHOLE_SIZE);
        buffers[SceneRT::DescriptorSetType::eTexCoords].emplace_back(texCoordsBuffer, 0, VK_WHOLE_SIZE);
    }

    static GeometryData CreateGeometryData(const tinygltf::Model& model)
    {
        GeometryBuffers geometryBuffers;

        Details::EnumerateNodes(model, [&](int32_t nodeIndex, const glm::mat4&)
            {
                const tinygltf::Node& node = model.nodes[nodeIndex];

                if (node.mesh >= 0)
                {
                    const tinygltf::Mesh& mesh = model.meshes[node.mesh];

                    for (const auto& primitive : mesh.primitives)
                    {
                        AppendPrimitiveGeometryBuffers(model, primitive, geometryBuffers);
                    }
                }
            });

        SceneRT::DescriptorSets descriptorSets;
        std::vector<vk::Buffer> buffers;

        for (const auto& [type, bufferInfo] : geometryBuffers)
        {
            const DescriptorDescription descriptorDescription{
                static_cast<uint32_t>(bufferInfo.size()),
                vk::DescriptorType::eStorageBuffer,
                vk::ShaderStageFlagBits::eClosestHitKHR,
                vk::DescriptorBindingFlagBits::eVariableDescriptorCount
            };

            const DescriptorData descriptorData{
                vk::DescriptorType::eStorageBuffer,
                bufferInfo
            };

            const DescriptorSet descriptorSet = DescriptorHelpers::CreateDescriptorSet(
                    { descriptorDescription }, { descriptorData });

            descriptorSets.emplace(type, descriptorSet);

            buffers.reserve(buffers.size() + bufferInfo.size());
            for (const auto& info : bufferInfo)
            {
                buffers.push_back(info.buffer);
            }
        }

        return GeometryData{ descriptorSets, buffers };
    }

    static TexturesData CreateTexturesData(const tinygltf::Model& model)
    {
        const std::vector<Texture> textures = Details::CreateTextures(model);
        const std::vector<vk::Sampler> samplers = Details::CreateSamplers(model);

        ImageInfo descriptorImageInfo;
        descriptorImageInfo.reserve(model.textures.size());

        for (const auto& texture : model.textures)
        {
            vk::Sampler sampler = Renderer::defaultSampler;

            if (texture.sampler >= 0)
            {
                sampler = samplers[texture.sampler];
            }

            descriptorImageInfo.emplace_back(sampler, textures[texture.source].view,
                    vk::ImageLayout::eShaderReadOnlyOptimal);
        }

        if (descriptorImageInfo.empty())
        {
            descriptorImageInfo.emplace_back(Renderer::defaultSampler,
                    Renderer::blackTexture.view, vk::ImageLayout::eShaderReadOnlyOptimal);
        }

        const DescriptorDescription descriptorDescription{
            static_cast<uint32_t>(descriptorImageInfo.size()),
            vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eRaygenKHR,
            vk::DescriptorBindingFlagBits::eVariableDescriptorCount
        };

        const DescriptorData descriptorData{
            vk::DescriptorType::eCombinedImageSampler,
            descriptorImageInfo
        };

        const DescriptorSet descriptorSet = DescriptorHelpers::CreateDescriptorSet(
                { descriptorDescription }, { descriptorData });

        return TexturesData{ descriptorSet, textures, samplers };
    }

    static MaterialsData CreateMaterialsData(const tinygltf::Model& model)
    {
        std::vector<MaterialRT> materialsData;

        for (const auto& material : model.materials)
        {
            Assert(material.pbrMetallicRoughness.baseColorTexture.texCoord == 0);
            Assert(material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord == 0);
            Assert(material.normalTexture.texCoord == 0);
            Assert(material.emissiveTexture.texCoord == 0);

            materialsData.push_back(MaterialRT{
                material.pbrMetallicRoughness.baseColorTexture.index,
                material.pbrMetallicRoughness.metallicRoughnessTexture.index,
                material.normalTexture.index,
                material.emissiveTexture.index,
                Helpers::GetVec<4>(material.pbrMetallicRoughness.baseColorFactor),
                Helpers::GetVec<4>(material.emissiveFactor),
                static_cast<float>(material.pbrMetallicRoughness.roughnessFactor),
                static_cast<float>(material.pbrMetallicRoughness.metallicFactor),
                static_cast<float>(material.normalTexture.scale),
                {}
            });
        }

        const vk::BufferUsageFlags bufferUsage = vk::BufferUsageFlagBits::eUniformBuffer;

        const vk::Buffer buffer = BufferHelpers::CreateDeviceLocalBufferWithData(
                bufferUsage, ByteView(materialsData), SyncScope::kRayTracingShaderRead);

        return MaterialsData{ buffer };
    }

    static DescriptorSet CreateTlasDescriptorSet(const AccelerationData& accelerationData,
            vk::ShaderStageFlags shaderStages = vk::ShaderStageFlagBits::eRaygenKHR)
    {
        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eAccelerationStructureKHR,
            shaderStages, vk::DescriptorBindingFlags()
        };

        const DescriptorData descriptorData = DescriptorHelpers::GetData(accelerationData.tlas);

        return DescriptorHelpers::CreateDescriptorSet({ descriptorDescription }, { descriptorData });
    }

    static DescriptorSet CreateMaterialsDescriptorSet(const MaterialsData& materialsData)
    {
        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eRaygenKHR,
            vk::DescriptorBindingFlags()
        };

        const DescriptorData descriptorData = DescriptorHelpers::GetData(materialsData.buffer);

        return DescriptorHelpers::CreateDescriptorSet({ descriptorDescription }, { descriptorData });
    }
}

SceneModel::SceneModel(const Filepath& path)
{
    model = std::make_unique<tinygltf::Model>();

    tinygltf::TinyGLTF loader;
    std::string errors;
    std::string warnings;

    const bool result = loader.LoadASCIIFromFile(model.get(), &errors, &warnings, path.GetAbsolute());

    if (!warnings.empty())
    {
        LogW << "Scene loaded with warnings:\n" << warnings;
    }

    if (!errors.empty())
    {
        LogE << "Failed to load scene:\n" << errors;
    }

    Assert(result);
}

SceneModel::~SceneModel() = default;

std::unique_ptr<Scene> SceneModel::CreateScene() const
{
    ScopeTime scopeTime("SceneModel::CreateScene");

    const DetailsRT::AccelerationData accelerationData = DetailsRT::CreateAccelerationData(*model);

    const Scene::Hierarchy sceneHierarchy{
        Details::CreateMeshes(*model),
        Details::CreateMaterials(*model),
        Details::CreateRenderObjects(*model),
    };

    Scene::Resources sceneResources;
    sceneResources.accelerationStructures = accelerationData.blases;
    sceneResources.accelerationStructures.push_back(accelerationData.tlas);
    sceneResources.buffers = Details::CollectBuffers(sceneHierarchy);
    sceneResources.samplers = Details::CreateSamplers(*model);
    sceneResources.textures = Details::CreateTextures(*model);

    Scene::DescriptorSets sceneDescriptorSets{
        DetailsRT::CreateTlasDescriptorSet(accelerationData, vk::ShaderStageFlagBits::eFragment),
        Details::CreateMaterialsDescriptorSet(*model, sceneHierarchy, sceneResources)
    };

    const Scene::Description sceneDescription{
        sceneHierarchy,
        sceneResources,
        sceneDescriptorSets
    };

    Scene* scene = new Scene(sceneDescription);

    return std::unique_ptr<Scene>(scene);
}

std::unique_ptr<SceneRT> SceneModel::CreateSceneRT() const
{
    ScopeTime scopeTime("SceneModel::CreateSceneRT");

    const SceneRT::Info sceneInfo{
        static_cast<uint32_t>(model->materials.size())
    };

    const DetailsRT::AccelerationData accelerationData = DetailsRT::CreateAccelerationData(*model);
    const DetailsRT::MaterialsData materialsData = DetailsRT::CreateMaterialsData(*model);

    auto [geometryDescriptorSets, geometryBuffers] = DetailsRT::CreateGeometryData(*model);
    auto [texturesDescriptorSet, textures, samplers] = DetailsRT::CreateTexturesData(*model);

    const DescriptorSet tlasDescriptorSet = DetailsRT::CreateTlasDescriptorSet(accelerationData);
    const DescriptorSet materialsDescriptorSet = DetailsRT::CreateMaterialsDescriptorSet(materialsData);

    SceneRT::Resources sceneResources;
    sceneResources.accelerationStructures = accelerationData.blases;
    sceneResources.accelerationStructures.push_back(accelerationData.tlas);
    sceneResources.buffers = std::move(geometryBuffers);
    sceneResources.buffers.push_back(materialsData.buffer);
    sceneResources.samplers = std::move(samplers);
    sceneResources.textures = std::move(textures);

    SceneRT::DescriptorSets sceneDescriptorSets;
    sceneDescriptorSets.insert(geometryDescriptorSets.begin(), geometryDescriptorSets.end());
    sceneDescriptorSets.emplace(SceneRT::DescriptorSetType::eTlas, tlasDescriptorSet);
    sceneDescriptorSets.emplace(SceneRT::DescriptorSetType::eMaterials, materialsDescriptorSet);
    sceneDescriptorSets.emplace(SceneRT::DescriptorSetType::eTextures, texturesDescriptorSet);

    const SceneRT::Description sceneDescription{
        sceneInfo,
        sceneResources,
        sceneDescriptorSets
    };

    SceneRT* scene = new SceneRT(sceneDescription);

    return std::unique_ptr<SceneRT>(scene);
}

std::unique_ptr<Camera> SceneModel::CreateCamera() const
{
    std::optional<Camera::Description> cameraDescription;

    Details::EnumerateNodes(*model, [&](int32_t nodeIndex, const glm::mat4&)
        {
            const tinygltf::Node& node = model->nodes[nodeIndex];

            if (node.camera >= 0 && !cameraDescription.has_value())
            {
                if (model->cameras[node.camera].type == "perspective")
                {
                    const tinygltf::PerspectiveCamera& perspectiveCamera = model->cameras[node.camera].perspective;

                    Assert(perspectiveCamera.aspectRatio != 0.0);
                    Assert(perspectiveCamera.zfar > perspectiveCamera.znear);

                    glm::quat rotation = glm::quat();
                    if (!node.rotation.empty())
                    {
                        rotation = Helpers::GetQuaternion(node.rotation);
                    }

                    const glm::vec3 position = Helpers::GetVec<3>(node.translation);
                    const glm::vec3 direction = rotation * Direction::kForward;
                    const glm::vec3 up = rotation * Direction::kUp;

                    cameraDescription = Camera::Description{
                        position, position + direction, up,
                        static_cast<float>(perspectiveCamera.yfov),
                        static_cast<float>(perspectiveCamera.aspectRatio),
                        static_cast<float>(perspectiveCamera.znear),
                        static_cast<float>(perspectiveCamera.zfar),
                    };
                }
            }
        });

    return std::make_unique<Camera>(cameraDescription.value_or(Config::DefaultCamera::kDescription));
}
