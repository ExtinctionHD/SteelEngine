#pragma warning(push, 0)
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#include <tiny_gltf.h>
#pragma warning(pop)

#include "Engine/Scene/SceneModel.hpp"

#include "Engine/Scene/SceneRT.hpp"
#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Shaders/PathTracing/PathTracing.h"

#include "Utils/Assert.hpp"

namespace SSceneModel
{
    using NodeFunctor = std::function<void(int32_t, const glm::mat4 &)>;

    using SceneData = std::pair<SceneRT::Descriptors, std::vector<vk::Buffer>>;

    using TexturesData = std::tuple<DescriptorSet, std::vector<Texture>, std::vector<vk::Sampler>>;

    namespace Vk
    {
        vk::Format GetFormat(const tinygltf::Image &image)
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

        vk::Filter GetSamplerFilter(int32_t filter)
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

        vk::SamplerMipmapMode GetSamplerMipmapMode(int32_t filter)
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

        vk::SamplerAddressMode GetSamplerAddressMode(int32_t wrap)
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

        vk::IndexType GetIndexType(int32_t type)
        {
            switch (type)
            {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                return vk::IndexType::eUint16;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                return vk::IndexType::eUint32;
            default:
                return vk::IndexType::eNoneNV;
            }
        }
    }

    namespace Math
    {
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

        std::vector<glm::vec3> CalculateTangents(const DataView<uint32_t> &indices,
                const DataView<glm::vec3> &positions, const DataView<glm::vec2> &texCoords)
        {
            std::vector<glm::vec3> tangents(positions.size);

            for (size_t i = 0; i < indices.size; i = i + 3)
            {
                const glm::vec3 &position0 = positions.data[indices.data[i]];
                const glm::vec3 &position1 = positions.data[indices.data[i + 1]];
                const glm::vec3 &position2 = positions.data[indices.data[i + 2]];

                const glm::vec3 edge1 = position1 - position0;
                const glm::vec3 edge2 = position2 - position0;

                const glm::vec2 &texCoord0 = texCoords.data[indices.data[i]];
                const glm::vec2 &texCoord1 = texCoords.data[indices.data[i + 1]];
                const glm::vec2 &texCoord2 = texCoords.data[indices.data[i + 2]];

                const glm::vec2 deltaTexCoord1 = texCoord1 - texCoord0;
                const glm::vec2 deltaTexCoord2 = texCoord2 - texCoord0;

                const float r = 1.0f / (deltaTexCoord1.x * deltaTexCoord2.y - deltaTexCoord1.y * deltaTexCoord2.x);

                const glm::vec3 tangent(
                        ((edge1.x * deltaTexCoord2.y) - (edge2.x * deltaTexCoord1.y)) * r,
                        ((edge1.y * deltaTexCoord2.y) - (edge2.y * deltaTexCoord1.y)) * r,
                        ((edge1.z * deltaTexCoord2.y) - (edge2.z * deltaTexCoord1.y)) * r);

                tangents[indices.data[i]] += tangent;
                tangents[indices.data[i + 1]] += tangent;
                tangents[indices.data[i + 2]] += tangent;
            }

            for (auto &tangent : tangents)
            {
                tangent = glm::normalize(tangent);
            }

            return tangents;
        }
    }

    namespace Data
    {
        constexpr vk::BufferUsageFlags kSceneDataUsage
                = vk::BufferUsageFlagBits::eRayTracingNV
                | vk::BufferUsageFlagBits::eStorageBuffer;

        using GeometryBuffers = std::map<SceneRT::DescriptorType, BufferInfo>;

        template <class T>
        DataView<T> GetAccessorDataView(const tinygltf::Model &model, const tinygltf::Accessor &accessor)
        {
            const tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
            Assert(bufferView.byteStride == 0);

            const size_t offset = bufferView.byteOffset + accessor.byteOffset;
            const T *data = reinterpret_cast<const T*>(model.buffers[bufferView.buffer].data.data() + offset);

            return DataView<T>(data, accessor.count);
        }

        ByteView GetAccessorByteView(const tinygltf::Model &model, const tinygltf::Accessor &accessor)
        {
            const tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
            Assert(bufferView.byteStride == 0);

            const size_t offset = bufferView.byteOffset + accessor.byteOffset;
            const uint8_t *data = model.buffers[bufferView.buffer].data.data() + offset;

            return DataView<uint8_t>(data, bufferView.byteLength - accessor.byteOffset);
        }

        vk::Buffer CreateBufferWithData(vk::BufferUsageFlags bufferUsage, const ByteView &data,
                const SyncScope &blockScope)
        {
            const BufferDescription bufferDescription{
                data.size,
                bufferUsage | vk::BufferUsageFlagBits::eTransferDst,
                vk::MemoryPropertyFlagBits::eDeviceLocal
            };

            const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(
                    bufferDescription, BufferCreateFlagBits::eStagingBuffer);

            VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
                {
                    BufferHelpers::UpdateBuffer(commandBuffer, buffer, data, blockScope);
                });

            return buffer;
        }

        glm::mat4 ExtractTransform(const tinygltf::Node &node)
        {
            if (!node.matrix.empty())
            {
                return glm::make_mat4(node.matrix.data());
            }

            glm::mat4 scaleMatrix(1.0f);
            if (!node.scale.empty())
            {
                const glm::vec3 scale = Math::GetVector3(node.scale);
                scaleMatrix = glm::scale(Matrix4::kIdentity, scale);
            }

            glm::mat4 rotationMatrix(1.0f);
            if (!node.rotation.empty())
            {
                const glm::quat rotation = Math::GetQuaternion(node.rotation);
                rotationMatrix = glm::toMat4(rotation);
            }

            glm::mat4 translationMatrix(1.0f);
            if (!node.translation.empty())
            {
                const glm::vec3 translation = Math::GetVector3(node.translation);
                translationMatrix = glm::translate(Matrix4::kIdentity, translation);
            }

            return translationMatrix * rotationMatrix * scaleMatrix;
        }

        void EnumerateNodes(const tinygltf::Model &model, const NodeFunctor &functor)
        {
            const NodeFunctor enumerator = [&](int32_t nodeIndex, const glm::mat4 &nodeTransform)
                {
                    const tinygltf::Node &node = model.nodes[nodeIndex];

                    for (const auto &childIndex : node.children)
                    {
                        const glm::mat4 childTransform = ExtractTransform(model.nodes[childIndex]);

                        enumerator(childIndex, nodeTransform * childTransform);
                    }

                    functor(nodeIndex, nodeTransform);
                };

            for (const auto &scene : model.scenes)
            {
                for (const auto &nodeIndex : scene.nodes)
                {
                    enumerator(nodeIndex, Matrix4::kIdentity);
                }
            }
        }

        void AppendPrimitiveGeometryBuffers(const tinygltf::Model &model,
                const tinygltf::Primitive &primitive, GeometryBuffers &buffers)
        {
            Assert(primitive.mode == TINYGLTF_MODE_TRIANGLES);
            Assert(primitive.indices != -1);

            const tinygltf::Accessor &indexAccessor = model.accessors[primitive.indices];

            DataView<uint32_t> indicesData = GetAccessorDataView<uint32_t>(model, indexAccessor);
            std::vector<uint32_t> indices;
            if (indexAccessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
            {
                Assert(indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);

                indices.resize(indexAccessor.count);

                for (size_t i = 0; i < indexAccessor.count; ++i)
                {
                    indices[i] = reinterpret_cast<const uint16_t*>(indicesData.data)[i];
                }

                indicesData = DataView(indices);
            }

            const tinygltf::Accessor &positionsAccessor = model.accessors[primitive.attributes.at("POSITION")];
            const DataView<glm::vec3> positionsData = GetAccessorDataView<glm::vec3>(model, positionsAccessor);

            const tinygltf::Accessor &normalsAccessor = model.accessors[primitive.attributes.at("NORMAL")];
            const DataView<glm::vec3> normalsData = GetAccessorDataView<glm::vec3>(model, normalsAccessor);

            const tinygltf::Accessor &texCoordsAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
            const DataView<glm::vec2> texCoordsData = GetAccessorDataView<glm::vec2>(model, texCoordsAccessor);

            Assert(primitive.attributes.count("TEXCOORD_1") == 0);

            DataView<glm::vec3> tangentsData;
            std::vector<glm::vec3> tangents;
            if (primitive.attributes.count("TANGENT") > 0)
            {
                const tinygltf::Accessor &tangentsAccessor = model.accessors[primitive.attributes.at("TANGENT")];
                tangentsData = GetAccessorDataView<glm::vec3>(model, tangentsAccessor);
            }
            else
            {
                tangents = Math::CalculateTangents(indicesData, positionsData, texCoordsData);
                tangentsData = DataView(tangents);
            }

            const SyncScope blockScope = SyncScope::kRayTracingShaderRead;

            const vk::Buffer indicesBuffer = CreateBufferWithData(kSceneDataUsage, indicesData, blockScope);
            const vk::Buffer positionsBuffer = CreateBufferWithData(kSceneDataUsage, positionsData, blockScope);
            const vk::Buffer normalsBuffer = CreateBufferWithData(kSceneDataUsage, normalsData, blockScope);
            const vk::Buffer tangentsBuffer = CreateBufferWithData(kSceneDataUsage, tangentsData, blockScope);
            const vk::Buffer texCoordsBuffer = CreateBufferWithData(kSceneDataUsage, texCoordsData, blockScope);

            buffers[SceneRT::DescriptorType::eIndices].emplace_back(indicesBuffer, 0, VK_WHOLE_SIZE);
            buffers[SceneRT::DescriptorType::ePositions].emplace_back(positionsBuffer, 0, VK_WHOLE_SIZE);
            buffers[SceneRT::DescriptorType::eNormals].emplace_back(normalsBuffer, 0, VK_WHOLE_SIZE);
            buffers[SceneRT::DescriptorType::eTangents].emplace_back(tangentsBuffer, 0, VK_WHOLE_SIZE);
            buffers[SceneRT::DescriptorType::eTexCoords].emplace_back(texCoordsBuffer, 0, VK_WHOLE_SIZE);
        }

        GeometryVertices CreateGeometryPositions(const tinygltf::Model &model,
                const tinygltf::Primitive &primitive)
        {
            Assert(primitive.mode == TINYGLTF_MODE_TRIANGLES);

            const tinygltf::Accessor accessor = model.accessors[primitive.attributes.at("POSITION")];
            const DataView<glm::vec3> data = GetAccessorDataView<glm::vec3>(model, accessor);

            const vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eRayTracingNV;
            const SyncScope blockScope = SyncScope::kAccelerationStructureBuild;
            const vk::Buffer buffer = CreateBufferWithData(usage, data, blockScope);

            const GeometryVertices vertices{
                buffer,
                vk::Format::eR32G32B32Sfloat,
                static_cast<uint32_t>(accessor.count),
                sizeof(glm::vec3)
            };

            return vertices;
        }

        GeometryIndices CreateGeometryIndices(const tinygltf::Model &model,
                const tinygltf::Primitive &primitive)
        {
            Assert(primitive.indices != -1);

            const tinygltf::Accessor accessor = model.accessors[primitive.indices];
            const ByteView data = GetAccessorByteView(model, accessor);

            const vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eRayTracingNV;
            const SyncScope blockScope = SyncScope::kAccelerationStructureBuild;
            const vk::Buffer buffer = CreateBufferWithData(usage, data, blockScope);

            const GeometryIndices indices{
                buffer,
                Vk::GetIndexType(accessor.componentType),
                static_cast<uint32_t>(accessor.count)
            };

            return indices;
        }

        std::vector<vk::AccelerationStructureNV> GenerateBlases(const tinygltf::Model &model)
        {
            std::vector<vk::AccelerationStructureNV> blases;
            blases.reserve(model.meshes.size());

            for (const auto &mesh : model.meshes)
            {
                for (const auto &primitive : mesh.primitives)
                {
                    const GeometryVertices vertices = CreateGeometryPositions(model, primitive);
                    const GeometryIndices indices = CreateGeometryIndices(model, primitive);

                    blases.push_back(VulkanContext::accelerationStructureManager->GenerateBlas(vertices, indices));

                    VulkanContext::bufferManager->DestroyBuffer(vertices.buffer);
                    VulkanContext::bufferManager->DestroyBuffer(indices.buffer);
                }
            }

            return blases;
        }

        std::vector<Texture> CreateTextures(const tinygltf::Model &model)
        {
            std::vector<Texture> textures;
            textures.reserve(model.images.size());

            for (const auto &image : model.images)
            {
                const vk::Format format = Vk::GetFormat(image);
                const vk::Extent2D extent = VulkanHelpers::GetExtent(image.width, image.height);

                textures.push_back(VulkanContext::textureManager->CreateTexture(format, extent, ByteView(image.image)));
            }

            return textures;
        }

        std::vector<vk::Sampler> CreateSamplers(const tinygltf::Model &model)
        {
            std::vector<vk::Sampler> samplers;
            samplers.reserve(model.samplers.size());

            for (const auto &sampler : model.samplers)
            {
                Assert(sampler.wrapS == sampler.wrapR);

                const SamplerDescription samplerDescription{
                    Vk::GetSamplerFilter(sampler.magFilter),
                    Vk::GetSamplerFilter(sampler.minFilter),
                    Vk::GetSamplerMipmapMode(sampler.magFilter),
                    Vk::GetSamplerAddressMode(sampler.wrapS),
                    VulkanConfig::kMaxAnisotropy,
                    0.0f, std::numeric_limits<float>::max()
                };

                samplers.push_back(VulkanContext::textureManager->CreateSampler(samplerDescription));
            }

            return samplers;
        }
    }

    vk::AccelerationStructureNV GenerateTlas(const tinygltf::Model &model)
    {
        const std::vector<vk::AccelerationStructureNV> blases = Data::GenerateBlases(model);

        std::vector<GeometryInstance> instances;

        Data::EnumerateNodes(model, [&](int32_t nodeIndex, const glm::mat4 &nodeTransform)
            {
                const tinygltf::Node &node = model.nodes[nodeIndex];

                if (node.mesh != -1)
                {
                    const tinygltf::Mesh &mesh = model.meshes[node.mesh];

                    for (size_t i = 0; i < mesh.primitives.size(); ++i)
                    {
                        const GeometryInstance instance{
                            blases[node.mesh + i],
                            nodeTransform
                        };

                        instances.push_back(instance);
                    }
                }
            });

        const vk::AccelerationStructureNV tlas = VulkanContext::accelerationStructureManager->GenerateTlas(instances);

        for (const auto &blas : blases)
        {
            VulkanContext::accelerationStructureManager->DestroyAccelerationStructure(blas);
        }

        return tlas;
    }

    SceneData CreateGeometryData(const tinygltf::Model &model)
    {
        Data::GeometryBuffers geometryBuffers;

        Data::EnumerateNodes(model, [&](int32_t nodeIndex, const glm::mat4 &)
            {
                const tinygltf::Node &node = model.nodes[nodeIndex];

                if (node.mesh != -1)
                {
                    const tinygltf::Mesh &mesh = model.meshes[node.mesh];

                    for (const auto &primitive : mesh.primitives)
                    {
                        Data::AppendPrimitiveGeometryBuffers(model, primitive, geometryBuffers);
                    }
                }
            });

        SceneRT::Descriptors descriptors;
        std::vector<vk::Buffer> buffers;

        for (const auto &[type, bufferInfo] : geometryBuffers)
        {
            const DescriptorDescription descriptorDescription{
                vk::DescriptorType::eStorageBuffer,
                static_cast<uint32_t>(bufferInfo.size()),
                vk::ShaderStageFlagBits::eRaygenNV,
                vk::DescriptorBindingFlagBits::eVariableDescriptorCount
            };

            const DescriptorData descriptorData{
                vk::DescriptorType::eStorageBuffer,
                bufferInfo
            };

            const DescriptorSet descriptor = DescriptorHelpers::CreateDescriptorSet(
                    { descriptorDescription }, { descriptorData });

            descriptors.emplace(type, descriptor);

            buffers.reserve(buffers.size() + bufferInfo.size());
            for (const auto &info : bufferInfo)
            {
                buffers.push_back(info.buffer);
            }
        }

        return std::make_pair(descriptors, buffers);
    }

    SceneData CreateMaterialsData(const tinygltf::Model &model)
    {
        std::vector<ShaderData::Material> materialsData;

        for (const auto &material : model.materials)
        {
            const ShaderData::Material materialData{
                material.pbrMetallicRoughness.baseColorTexture.index,
                material.pbrMetallicRoughness.metallicRoughnessTexture.index,
                material.normalTexture.index,
                material.emissiveTexture.index,
                Math::GetVector4(material.pbrMetallicRoughness.baseColorFactor),
                Math::GetVector3(material.emissiveFactor),
                static_cast<float>(material.pbrMetallicRoughness.roughnessFactor),
                static_cast<float>(material.pbrMetallicRoughness.metallicFactor),
                static_cast<float>(material.normalTexture.scale)
            };

            materialsData.push_back(materialData);
        }

        const vk::Buffer buffer = Data::CreateBufferWithData(Data::kSceneDataUsage,
                ByteView(materialsData), SyncScope::kRayTracingShaderRead);

        const DescriptorDescription descriptorDescription{
            vk::DescriptorType::eStorageBuffer,
            1, vk::ShaderStageFlagBits::eRaygenNV,
            vk::DescriptorBindingFlags()
        };

        const DescriptorSet descriptor = DescriptorHelpers::CreateDescriptorSet(
                { descriptorDescription }, { DescriptorHelpers::GetData(buffer) });

        const SceneRT::Descriptors descriptors{
            { SceneRT::DescriptorType::eMaterials, descriptor }
        };

        return std::make_pair(descriptors, std::vector<vk::Buffer>{ buffer });
    }

    TexturesData CreateTexturesData(const tinygltf::Model &model)
    {
        const std::vector<Texture> textures = Data::CreateTextures(model);
        const std::vector<vk::Sampler> samplers = Data::CreateSamplers(model);

        ImageInfo descriptorImageInfo;
        descriptorImageInfo.reserve(model.textures.size());

        for (const auto &texture : model.textures)
        {
            descriptorImageInfo.emplace_back(samplers[texture.sampler],
                    textures[texture.source].view, vk::ImageLayout::eShaderReadOnlyOptimal);
        }

        const DescriptorDescription descriptorDescription{
            vk::DescriptorType::eCombinedImageSampler,
            static_cast<uint32_t>(descriptorImageInfo.size()),
            vk::ShaderStageFlagBits::eClosestHitNV,
            vk::DescriptorBindingFlagBits::eVariableDescriptorCount
        };

        const DescriptorData descriptorData{
            vk::DescriptorType::eCombinedImageSampler,
            descriptorImageInfo
        };

        const DescriptorSet descriptor = DescriptorHelpers::CreateDescriptorSet(
                { descriptorDescription }, { descriptorData });

        return std::make_tuple(descriptor, textures, samplers);
    }
}

SceneModel::SceneModel(const Filepath &path)
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

std::unique_ptr<SceneRT> SceneModel::CreateSceneRT() const
{
    std::unique_ptr<SceneRT> scene = std::make_unique<SceneRT>();

    scene->tlas = SSceneModel::GenerateTlas(*model);

    const auto [geometryDescriptors, geometryBuffers] = SSceneModel::CreateGeometryData(*model);
    const auto [materialsDescriptor, materialsBuffer] = SSceneModel::CreateMaterialsData(*model);

    scene->descriptors.insert(geometryDescriptors.begin(), geometryDescriptors.end());
    scene->descriptors.insert(materialsDescriptor.begin(), materialsDescriptor.end());

    scene->buffers.reserve(geometryBuffers.size() + materialsBuffer.size());
    scene->buffers.insert(scene->buffers.begin(), geometryBuffers.begin(), geometryBuffers.end());
    scene->buffers.insert(scene->buffers.begin(), materialsBuffer.begin(), materialsBuffer.end());

    DescriptorSet &texturesDescriptor = scene->descriptors[SceneRT::DescriptorType::eTextures];
    std::tie(texturesDescriptor, scene->textures, scene->samplers) = SSceneModel::CreateTexturesData(*model);

    return scene;
}
