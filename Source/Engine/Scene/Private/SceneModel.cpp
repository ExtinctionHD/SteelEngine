#pragma warning(push, 0)
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#include <tiny_gltf.h>
#pragma warning(pop)

#include "Engine/Scene/SceneModel.hpp"

#include "Engine/Camera.hpp"
#include "Engine/Scene/SceneRT.hpp"
#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/EngineHelpers.hpp"
#include "Engine/Config.hpp"

#include "Shaders/PathTracing/PathTracing.h"

#include "Utils/Assert.hpp"

namespace Details
{
    using NodeFunctor = std::function<void(int32_t, const glm::mat4 &)>;

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

    struct CameraData
    {
        Camera *camera;
        vk::Buffer buffer;
    };

    struct MaterialsData
    {
        vk::Buffer buffer;
    };

    struct EnvironmentData
    {
        Texture texture;
        vk::Sampler sampler;
    };

    struct GeneralData
    {
        AccelerationData accelerationData;
        CameraData cameraData;
        MaterialsData materialsData;
        EnvironmentData environmentData;
    };

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
                return vk::IndexType::eNoneKHR;
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
        using AccelerationStructures = std::vector<vk::AccelerationStructureKHR>;

        using GeometryBuffers = std::map<SceneRT::DescriptorSetType, BufferInfo>;

        uint32_t GetCustomIndex(uint16_t instanceIndex, uint8_t materialIndex)
        {
            return static_cast<uint32_t>(instanceIndex) | (static_cast<uint32_t>(materialIndex) << 16);
        }

        template <class T>
        DataView<T> GetAccessorDataView(const tinygltf::Model &model,
                const tinygltf::Accessor &accessor)
        {
            const tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
            Assert(bufferView.byteStride == 0);

            const size_t offset = bufferView.byteOffset + accessor.byteOffset;
            const T *data = reinterpret_cast<const T*>(model.buffers[bufferView.buffer].data.data() + offset);

            return DataView<T>(data, accessor.count);
        }

        ByteView GetAccessorByteView(const tinygltf::Model &model,
                const tinygltf::Accessor &accessor)
        {
            const tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
            Assert(bufferView.byteStride == 0);

            const size_t offset = bufferView.byteOffset + accessor.byteOffset;
            const uint8_t *data = model.buffers[bufferView.buffer].data.data() + offset;

            return DataView<uint8_t>(data, bufferView.byteLength - accessor.byteOffset);
        }

        vk::Buffer CreateBufferWithData(vk::BufferUsageFlags bufferUsage,
                const ByteView &data, const SyncScope &blockScope)
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
            const NodeFunctor enumerator = [&](int32_t nodeIndex, const glm::mat4 &parentTransform)
                {
                    const tinygltf::Node &node = model.nodes[nodeIndex];
                    const glm::mat4 transform = parentTransform * ExtractTransform(node);

                    for (const auto &childIndex : node.children)
                    {
                        enumerator(childIndex, transform);
                    }

                    functor(nodeIndex, transform);
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
            Assert(primitive.indices >= 0);

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

            constexpr vk::BufferUsageFlags bufferUsage
                    = vk::BufferUsageFlagBits::eRayTracingKHR
                    | vk::BufferUsageFlagBits::eStorageBuffer;

            const SyncScope blockScope = SyncScope::kRayTracingShaderRead;

            const vk::Buffer indicesBuffer = CreateBufferWithData(bufferUsage, indicesData, blockScope);
            const vk::Buffer positionsBuffer = CreateBufferWithData(bufferUsage, positionsData, blockScope);
            const vk::Buffer normalsBuffer = CreateBufferWithData(bufferUsage, normalsData, blockScope);
            const vk::Buffer tangentsBuffer = CreateBufferWithData(bufferUsage, tangentsData, blockScope);
            const vk::Buffer texCoordsBuffer = CreateBufferWithData(bufferUsage, texCoordsData, blockScope);

            buffers[SceneRT::DescriptorSetType::eIndices].emplace_back(indicesBuffer, 0, VK_WHOLE_SIZE);
            buffers[SceneRT::DescriptorSetType::ePositions].emplace_back(positionsBuffer, 0, VK_WHOLE_SIZE);
            buffers[SceneRT::DescriptorSetType::eNormals].emplace_back(normalsBuffer, 0, VK_WHOLE_SIZE);
            buffers[SceneRT::DescriptorSetType::eTangents].emplace_back(tangentsBuffer, 0, VK_WHOLE_SIZE);
            buffers[SceneRT::DescriptorSetType::eTexCoords].emplace_back(texCoordsBuffer, 0, VK_WHOLE_SIZE);
        }

        GeometryVertexData CreateGeometryPositions(const tinygltf::Model &model,
                const tinygltf::Primitive &primitive)
        {
            Assert(primitive.mode == TINYGLTF_MODE_TRIANGLES);

            const tinygltf::Accessor accessor = model.accessors[primitive.attributes.at("POSITION")];
            const DataView<glm::vec3> data = GetAccessorDataView<glm::vec3>(model, accessor);

            const vk::BufferUsageFlags bufferUsage
                    = vk::BufferUsageFlagBits::eRayTracingKHR
                    | vk::BufferUsageFlagBits::eShaderDeviceAddressEXT;

            const SyncScope blockScope = SyncScope::kAccelerationStructureBuild;
            const vk::Buffer buffer = CreateBufferWithData(bufferUsage, data, blockScope);

            const GeometryVertexData vertices{
                buffer,
                vk::Format::eR32G32B32Sfloat,
                static_cast<uint32_t>(accessor.count),
                sizeof(glm::vec3)
            };

            return vertices;
        }

        GeometryIndexData CreateGeometryIndices(const tinygltf::Model &model,
                const tinygltf::Primitive &primitive)
        {
            Assert(primitive.indices >= 0);

            const tinygltf::Accessor accessor = model.accessors[primitive.indices];
            const ByteView data = GetAccessorByteView(model, accessor);

            const vk::BufferUsageFlags bufferUsage
                    = vk::BufferUsageFlagBits::eRayTracingKHR
                    | vk::BufferUsageFlagBits::eShaderDeviceAddressEXT;

            const SyncScope blockScope = SyncScope::kAccelerationStructureBuild;
            const vk::Buffer buffer = CreateBufferWithData(bufferUsage, data, blockScope);

            const GeometryIndexData indices{
                buffer,
                Vk::GetIndexType(accessor.componentType),
                static_cast<uint32_t>(accessor.count)
            };

            return indices;
        }

        AccelerationStructures GenerateBlases(const tinygltf::Model &model)
        {
            std::vector<vk::AccelerationStructureKHR> blases;
            blases.reserve(model.meshes.size());

            for (const auto &mesh : model.meshes)
            {
                for (const auto &primitive : mesh.primitives)
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

    AccelerationData CreateAccelerationData(const tinygltf::Model &model)
    {
        const std::vector<vk::AccelerationStructureKHR> blases = Data::GenerateBlases(model);

        std::vector<GeometryInstanceData> instances;

        Data::EnumerateNodes(model, [&](int32_t nodeIndex, const glm::mat4 &transform)
            {
                const tinygltf::Node &node = model.nodes[nodeIndex];

                if (node.mesh >= 0)
                {
                    const tinygltf::Mesh &mesh = model.meshes[node.mesh];

                    for (size_t i = 0; i < mesh.primitives.size(); ++i)
                    {
                        const uint16_t instanceIndex = static_cast<uint16_t>(instances.size());
                        const uint8_t materialIndex = static_cast<uint8_t>(mesh.primitives[i].material);

                        const vk::GeometryInstanceFlagsKHR flags
                                = vk::GeometryInstanceFlagBitsKHR::eTriangleFrontCounterclockwise
                                | vk::GeometryInstanceFlagBitsKHR::eForceOpaque;

                        const GeometryInstanceData instance{
                            blases[node.mesh + i], transform,
                            Data::GetCustomIndex(instanceIndex, materialIndex),
                            0xFF, 0, flags
                        };

                        instances.push_back(instance);
                    }
                }
            });

        const vk::AccelerationStructureKHR tlas = VulkanContext::accelerationStructureManager->GenerateTlas(instances);

        return AccelerationData{ tlas, blases };
    }

    GeometryData CreateGeometryData(const tinygltf::Model &model)
    {
        Data::GeometryBuffers geometryBuffers;

        Data::EnumerateNodes(model, [&](int32_t nodeIndex, const glm::mat4 &)
            {
                const tinygltf::Node &node = model.nodes[nodeIndex];

                if (node.mesh >= 0)
                {
                    const tinygltf::Mesh &mesh = model.meshes[node.mesh];

                    for (const auto &primitive : mesh.primitives)
                    {
                        Data::AppendPrimitiveGeometryBuffers(model, primitive, geometryBuffers);
                    }
                }
            });

        SceneRT::DescriptorSets descriptorSets;
        std::vector<vk::Buffer> buffers;

        for (const auto &[type, bufferInfo] : geometryBuffers)
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
            for (const auto &info : bufferInfo)
            {
                buffers.push_back(info.buffer);
            }
        }

        return GeometryData{ descriptorSets, buffers };
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

    CameraData CreateCameraData(const tinygltf::Model &model)
    {
        std::optional<Camera::Description> cameraDescription;

        Data::EnumerateNodes(model, [&](int32_t nodeIndex, const glm::mat4 &)
            {
                const tinygltf::Node &node = model.nodes[nodeIndex];

                if (node.camera >= 0 && !cameraDescription.has_value())
                {
                    if (model.cameras[node.camera].type == "perspective")
                    {
                        const tinygltf::PerspectiveCamera &perspectiveCamera = model.cameras[node.camera].perspective;

                        Assert(perspectiveCamera.aspectRatio != 0.0f);
                        Assert(perspectiveCamera.zfar > perspectiveCamera.znear);

                        const glm::vec3 position = Math::GetVector3(node.translation);
                        const glm::vec3 direction = Math::GetQuaternion(node.rotation) * Direction::kForward;
                        const glm::vec3 up = Math::GetQuaternion(node.rotation) * Direction::kUp;

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

        Camera *camera = new Camera(cameraDescription.value_or(Config::DefaultCamera::kDescription));

        const ShaderData::Camera cameraShaderData{
            glm::inverse(camera->GetViewMatrix()),
            glm::inverse(camera->GetProjectionMatrix()),
            cameraDescription->zNear,
            cameraDescription->zFar
        };

        const vk::BufferUsageFlags bufferUsage
                = vk::BufferUsageFlagBits::eRayTracingKHR
                | vk::BufferUsageFlagBits::eUniformBuffer;

        const vk::Buffer buffer = Data::CreateBufferWithData(bufferUsage,
                ByteView(cameraShaderData), SyncScope::kRayTracingShaderRead);

        return CameraData{ camera, buffer };
    }

    MaterialsData CreateMaterialsData(const tinygltf::Model &model)
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

        constexpr vk::BufferUsageFlags bufferUsage
                = vk::BufferUsageFlagBits::eRayTracingKHR
                | vk::BufferUsageFlagBits::eUniformBuffer;

        const vk::Buffer buffer = Data::CreateBufferWithData(bufferUsage,
                ByteView(materialsData), SyncScope::kRayTracingShaderRead);

        return MaterialsData{ buffer };
    }

    EnvironmentData CreateEnvironmentData(const Filepath &path)
    {
        TextureManager &textureManager = *VulkanContext::textureManager;
        ImageManager &imageManager = *VulkanContext::imageManager;

        const Texture panoramaTexture = textureManager.CreateTexture(path);
        const vk::Extent3D &panoramaExtent = imageManager.GetImageDescription(panoramaTexture.image).extent;

        const vk::Extent2D environmentExtent = vk::Extent2D(panoramaExtent.height / 2, panoramaExtent.height / 2);
        const Texture environmentTexture = textureManager.CreateCubeTexture(panoramaTexture, environmentExtent);

        textureManager.DestroyTexture(panoramaTexture);

        return EnvironmentData{ environmentTexture, textureManager.GetDefaultSampler() };
    }

    DescriptorSet CreateGeneralDescriptorSet(const GeneralData &generalData)
    {
        const DescriptorSetDescription descriptorSetDescription{
            DescriptorDescription{
                1, vk::DescriptorType::eAccelerationStructureKHR,
                vk::ShaderStageFlagBits::eRaygenKHR,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                1, vk::DescriptorType::eUniformBuffer,
                vk::ShaderStageFlagBits::eRaygenKHR,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                1, vk::DescriptorType::eUniformBuffer,
                vk::ShaderStageFlagBits::eRaygenKHR,
                vk::DescriptorBindingFlags()
            },
            DescriptorDescription{
                1, vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eMissKHR,
                vk::DescriptorBindingFlags()
            }
        };

        const auto &[environmentTexture, environmentSampler] = generalData.environmentData;

        const DescriptorSetData descriptorSetData{
            DescriptorHelpers::GetData(generalData.accelerationData.tlas),
            DescriptorHelpers::GetData(generalData.cameraData.buffer),
            DescriptorHelpers::GetData(generalData.materialsData.buffer),
            DescriptorHelpers::GetData(environmentSampler, environmentTexture.view)
        };

        return DescriptorHelpers::CreateDescriptorSet(descriptorSetDescription, descriptorSetData);
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

std::unique_ptr<SceneRT> SceneModel::CreateSceneRT(const Filepath &environmentPath) const
{
    const SceneRT::Info sceneInfo{
        static_cast<uint32_t>(model->materials.size())
    };

    Details::GeneralData generalData{
        Details::CreateAccelerationData(*model),
        Details::CreateCameraData(*model),
        Details::CreateMaterialsData(*model),
        Details::CreateEnvironmentData(environmentPath)
    };

    const DescriptorSet generalDescriptorSet = Details::CreateGeneralDescriptorSet(generalData);

    auto [geometryDescriptorSets, geometryBuffers] = Details::CreateGeometryData(*model);
    auto [texturesDescriptorSet, textures, samplers] = Details::CreateTexturesData(*model);

    SceneRT::Resources sceneResources;
    sceneResources.accelerationStructures = std::move(generalData.accelerationData.blases);
    sceneResources.accelerationStructures.push_back(generalData.accelerationData.tlas);
    sceneResources.buffers = std::move(geometryBuffers);
    sceneResources.buffers.push_back(generalData.materialsData.buffer);
    sceneResources.buffers.push_back(generalData.cameraData.buffer);
    sceneResources.samplers = std::move(samplers);
    sceneResources.samplers.push_back(generalData.environmentData.sampler);
    sceneResources.textures = std::move(textures);
    sceneResources.textures.push_back(generalData.environmentData.texture);

    const SceneRT::References sceneReferences{
        generalData.accelerationData.tlas,
        generalData.cameraData.buffer,
    };

    SceneRT::DescriptorSets sceneDescriptorSets;
    sceneDescriptorSets.insert(geometryDescriptorSets.begin(), geometryDescriptorSets.end());
    sceneDescriptorSets.emplace(SceneRT::DescriptorSetType::eGeneral, generalDescriptorSet);
    sceneDescriptorSets.emplace(SceneRT::DescriptorSetType::eTextures, texturesDescriptorSet);

    const SceneRT::Description sceneDescription{
        sceneInfo,
        sceneResources,
        sceneReferences,
        sceneDescriptorSets
    };

    SceneRT *scene = new SceneRT(generalData.cameraData.camera, sceneDescription);

    return std::unique_ptr<SceneRT>(scene);
}
