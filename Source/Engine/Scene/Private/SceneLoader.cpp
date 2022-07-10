#pragma warning(push, 0)
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#include <tiny_gltf.h>
#pragma warning(pop)

#include "Engine/Scene/SceneLoader.hpp"

#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Scene/SceneComponents.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Scene/Components.hpp"
#include "Engine/Scene/DirectLighting.hpp"
#include "Engine/Scene/ImageBasedLighting.hpp"
#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Primitive.hpp"
#include "Engine/Scene/Scene.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"

namespace Details
{
    using NodeFunctor = std::function<entt::entity(const tinygltf::Node&, entt::entity)>;

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
            Assert(false);
            return vk::Format::eUndefined;
        }
    }

    static vk::Filter GetSamplerFilter(int32_t filter)
    {
        switch (filter)
        {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            return vk::Filter::eNearest;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            return vk::Filter::eLinear;
        default:
            return vk::Filter::eLinear;
        }
    }

    static vk::SamplerMipmapMode GetSamplerMipmapMode(int32_t filter)
    {
        switch (filter)
        {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            return vk::SamplerMipmapMode::eNearest;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            return vk::SamplerMipmapMode::eLinear;
        default:
            return vk::SamplerMipmapMode::eLinear;
        }
    }

    static vk::SamplerAddressMode GetSamplerAddressMode(int32_t wrap)
    {
        switch (wrap)
        {
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            return vk::SamplerAddressMode::eRepeat;
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            return vk::SamplerAddressMode::eClampToEdge;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            return vk::SamplerAddressMode::eMirroredRepeat;
        default:
            return vk::SamplerAddressMode::eRepeat;
        }
    }

    static vk::IndexType GetIndexType(int32_t componentType)
    {
        switch (componentType)
        {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return vk::IndexType::eUint16;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            return vk::IndexType::eUint32;
        default:
            Assert(false);
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
        Assert(bufferView.byteStride == 0 || bufferView.byteStride == GetAccessorValueSize(accessor));

        const size_t offset = bufferView.byteOffset + accessor.byteOffset;
        const uint8_t* data = model.buffers[bufferView.buffer].data.data() + offset;

        return DataView<uint8_t>(data, bufferView.byteLength - accessor.byteOffset);
    }

    static void EnumerateNodes(const tinygltf::Model& model, const NodeFunctor& functor)
    {
        using Enumerator = std::function<void(const tinygltf::Node&, entt::entity)>;

        const Enumerator enumerator = [&](const tinygltf::Node& node, entt::entity parent)
            {
                const entt::entity entity = functor(node, parent);

                for (const auto& childIndex : node.children)
                {
                    const tinygltf::Node& child = model.nodes[childIndex];

                    enumerator(child, entity);
                }
            };

        for (const auto& scene : model.scenes)
        {
            for (const auto& nodeIndex : scene.nodes)
            {
                const tinygltf::Node& node = model.nodes[nodeIndex];

                enumerator(node, entt::null);
            }
        }
    }

    static std::vector<Texture> RetrieveImages(const tinygltf::Model& model)
    {
        std::vector<Texture> textures;
        textures.reserve(model.images.size());

        for (const auto& image : model.images)
        {
            const vk::Format format = GetFormat(image);
            const vk::Extent2D extent = VulkanHelpers::GetExtent(image.width, image.height);

            textures.push_back(VulkanContext::textureManager->CreateTexture(format, extent, ByteView(image.image)));
        }

        return textures;
    }

    static std::vector<vk::Sampler> RetrieveSamplers(const tinygltf::Model& model)
    {
        std::vector<vk::Sampler> samplers;
        samplers.reserve(model.samplers.size());

        for (const auto& sampler : model.samplers)
        {
            Assert(sampler.wrapS == sampler.wrapT);

            const SamplerDescription samplerDescription{
                GetSamplerFilter(sampler.magFilter),
                GetSamplerFilter(sampler.minFilter),
                GetSamplerMipmapMode(sampler.magFilter),
                GetSamplerAddressMode(sampler.wrapS),
                VulkanConfig::kMaxAnisotropy,
                0.0f, std::numeric_limits<float>::max(),
                false
            };

            samplers.push_back(VulkanContext::textureManager->CreateSampler(samplerDescription));
        }

        return samplers;
    }

    static Material RetrieveMaterial(const tinygltf::Material& gltfMaterial)
    {
        Assert(gltfMaterial.pbrMetallicRoughness.baseColorTexture.texCoord == 0);
        Assert(gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.texCoord == 0);
        Assert(gltfMaterial.normalTexture.texCoord == 0);
        Assert(gltfMaterial.occlusionTexture.texCoord == 0);
        Assert(gltfMaterial.emissiveTexture.texCoord == 0);

        Material material{};

        material.data.baseColorTexture = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
        material.data.roughnessMetallicTexture = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
        material.data.normalTexture = gltfMaterial.normalTexture.index;
        material.data.occlusionTexture = gltfMaterial.occlusionTexture.index;
        material.data.emissionTexture = gltfMaterial.emissiveTexture.index;

        material.data.baseColorFactor = GetVec<4>(gltfMaterial.pbrMetallicRoughness.baseColorFactor);
        material.data.emissionFactor = GetVec<4>(gltfMaterial.emissiveFactor);

        material.data.roughnessFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor);
        material.data.metallicFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor);
        material.data.normalScale = static_cast<float>(gltfMaterial.normalTexture.scale);
        material.data.occlusionStrength = static_cast<float>(gltfMaterial.occlusionTexture.strength);
        material.data.alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);

        if (gltfMaterial.alphaMode != "OPAQUE")
        {
            material.flags |= MaterialFlagBits::eAlphaTest;
        }
        if (gltfMaterial.doubleSided)
        {
            material.flags |= MaterialFlagBits::eDoubleSided;
        }
        if (gltfMaterial.normalTexture.index >= 0)
        {
            material.flags |= MaterialFlagBits::eNormalMapping;
        }

        return material;
    }

    static std::vector<Primitive::Vertex> RetrieveVertices(
            const tinygltf::Model& model, const tinygltf::Primitive& gltfPrimitive)
    {
        Assert(gltfPrimitive.attributes.contains("POSITION"));
        const tinygltf::Accessor& positionsAccessor = model.accessors[gltfPrimitive.attributes.at("POSITION")];

        Assert(positionsAccessor.type == TINYGLTF_TYPE_VEC3);
        Assert(positionsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        const DataView<glm::vec3> positions = GetAccessorDataView<glm::vec3>(model, positionsAccessor);

        DataView<glm::vec3> normals;
        if (gltfPrimitive.attributes.contains("NORMAL"))
        {
            const tinygltf::Accessor& normalsAccessor = model.accessors[gltfPrimitive.attributes.at("NORMAL")];

            Assert(normalsAccessor.type == TINYGLTF_TYPE_VEC3);
            Assert(normalsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            normals = GetAccessorDataView<glm::vec3>(model, normalsAccessor);
        }

        DataView<glm::vec3> tangents;
        if (gltfPrimitive.attributes.contains("TANGENT"))
        {
            const tinygltf::Accessor& tangentsAccessor = model.accessors[gltfPrimitive.attributes.at("TANGENT")];

            Assert(tangentsAccessor.type == TINYGLTF_TYPE_VEC3);
            Assert(tangentsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            tangents = GetAccessorDataView<glm::vec3>(model, tangentsAccessor);
        }

        DataView<glm::vec2> texCoords;
        if (gltfPrimitive.attributes.contains("TEXCOORD_0"))
        {
            const tinygltf::Accessor& texCoordsAccessor = model.accessors[gltfPrimitive.attributes.at("TEXCOORD_0")];

            Assert(texCoordsAccessor.type == TINYGLTF_TYPE_VEC2);
            Assert(texCoordsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            texCoords = GetAccessorDataView<glm::vec2>(model, texCoordsAccessor);
        }

        std::vector<Primitive::Vertex> vertices(positions.size);

        for (size_t i = 0; i < vertices.size(); ++i)
        {
            vertices[i].position = positions[i];

            if (normals.data != nullptr)
            {
                Assert(normals.size == vertices.size());
                vertices[i].normal = normals[i];
            }

            if (tangents.data != nullptr)
            {
                Assert(tangents.size == vertices.size());
                vertices[i].tangent = tangents[i];
            }

            if (texCoords.data != nullptr)
            {
                Assert(texCoords.size == vertices.size());
                vertices[i].texCoord = texCoords[i];
            }
        }

        return vertices;
    }

    static Primitive RetrievePrimitive(
            const tinygltf::Model& model, const tinygltf::Primitive& gltfPrimitive)
    {
        Assert(gltfPrimitive.indices >= 0);
        const tinygltf::Accessor& indicesAccessor = model.accessors[gltfPrimitive.indices];

        const vk::IndexType indexType = GetIndexType(indicesAccessor.componentType);
        const ByteView indices = GetAccessorByteView(model, indicesAccessor);

        std::vector<Primitive::Vertex> vertices = RetrieveVertices(model, gltfPrimitive);

        if (!gltfPrimitive.attributes.contains("NORMAL"))
        {
            PrimitiveHelpers::CalculateNormals(indexType, indices, vertices);
        }
        if (!gltfPrimitive.attributes.contains("TANGENT"))
        {
            PrimitiveHelpers::CalculateTangents(indexType, indices, vertices);
        }

        const uint32_t indexCount = static_cast<uint32_t>(indicesAccessor.count);

        const vk::Buffer indexBuffer = BufferHelpers::CreateBufferWithData(
                vk::BufferUsageFlagBits::eIndexBuffer, indices);

        const vk::Buffer vertexBuffer = BufferHelpers::CreateBufferWithData(
                vk::BufferUsageFlagBits::eVertexBuffer, ByteView(vertices));

        Primitive primitive;

        primitive.indexType = indexType;
        primitive.indexCount = indexCount;
        primitive.indexBuffer = indexBuffer;
        primitive.vertexBuffer = vertexBuffer;

        for (const auto& vertex : vertices)
        {
            primitive.bbox.Add(vertex.position);
        }

        return primitive;
    }

    static vk::AccelerationStructureKHR GenerateBlas(
            const tinygltf::Model& model, const tinygltf::Primitive& gltfPrimitive)
    {
        Assert(gltfPrimitive.indices >= 0);
        const tinygltf::Accessor& indicesAccessor = model.accessors[gltfPrimitive.indices];

        Assert(gltfPrimitive.attributes.contains("POSITION"));
        const tinygltf::Accessor& positionsAccessor = model.accessors[gltfPrimitive.attributes.at("POSITION")];

        Assert(positionsAccessor.type == TINYGLTF_TYPE_VEC3);
        Assert(positionsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

        BlasGeometryData geometryData;

        geometryData.indexType = GetIndexType(indicesAccessor.componentType);
        geometryData.indexCount = static_cast<uint32_t>(indicesAccessor.count);
        geometryData.indices = GetAccessorByteView(model, indicesAccessor);

        geometryData.vertexFormat = vk::Format::eR32G32B32Sfloat;
        geometryData.vertexStride = sizeof(glm::vec3);
        geometryData.vertexCount = static_cast<uint32_t>(positionsAccessor.count);
        geometryData.vertices = GetAccessorByteView(model, positionsAccessor);

        return VulkanContext::accelerationStructureManager->GenerateBlas(geometryData);
    }

    static vk::Buffer CreateRayTracingIndexBuffer(
            const tinygltf::Model& model, const tinygltf::Primitive& gltfPrimitive)
    {
        Assert(gltfPrimitive.indices >= 0);
        const tinygltf::Accessor& indicesAccessor = model.accessors[gltfPrimitive.indices];

        const vk::IndexType indexType = GetIndexType(indicesAccessor.componentType);

        const ByteView indices = GetAccessorByteView(model, indicesAccessor);

        if (indexType == vk::IndexType::eUint32)
        {
            return BufferHelpers::CreateBufferWithData(vk::BufferUsageFlagBits::eStorageBuffer, indices);
        }

        Assert(indexType == vk::IndexType::eUint16);

        const DataView<uint16_t> indices16 = DataView<uint16_t>(indices);

        std::vector<uint32_t> indices32(indices16.size);

        for (size_t i = 0; i < indices16.size; ++i)
        {
            indices32[i] = static_cast<uint32_t>(indices16[i]);
        }

        return BufferHelpers::CreateBufferWithData(vk::BufferUsageFlagBits::eStorageBuffer, ByteView(indices32));
    }

    static vk::Buffer CreateRayTracingVertexBuffer(
            const tinygltf::Model& model, const tinygltf::Primitive& gltfPrimitive)
    {
        Assert(gltfPrimitive.indices >= 0);
        const tinygltf::Accessor& indicesAccessor = model.accessors[gltfPrimitive.indices];

        const vk::IndexType indexType = GetIndexType(indicesAccessor.componentType);

        const ByteView indices = GetAccessorByteView(model, indicesAccessor);

        std::vector<Primitive::Vertex> vertices = RetrieveVertices(model, gltfPrimitive);

        if (!gltfPrimitive.attributes.contains("NORMAL"))
        {
            PrimitiveHelpers::CalculateNormals(indexType, indices, vertices);
        }
        if (!gltfPrimitive.attributes.contains("TANGENT"))
        {
            PrimitiveHelpers::CalculateTangents(indexType, indices, vertices);
        }

        std::vector<gpu::VertexRT> verticesRT(vertices.size());

        for (size_t i = 0; i < vertices.size(); ++i)
        {
            verticesRT[i].normal = glm::vec4(vertices[i].normal, vertices[i].texCoord.x);
            verticesRT[i].tangent = glm::vec4(vertices[i].tangent, vertices[i].texCoord.y);
        }

        return BufferHelpers::CreateBufferWithData(vk::BufferUsageFlagBits::eStorageBuffer, ByteView(verticesRT));
    }

    static glm::mat4 RetrieveTransform(const tinygltf::Node& node)
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

    static CameraLocation RetrieveCameraLocation(const tinygltf::Node& node)
    {
        glm::quat rotation = glm::quat();
        if (!node.rotation.empty())
        {
            rotation = GetQuaternion(node.rotation);
        }

        const glm::vec3 position = GetVec<3>(node.translation);
        const glm::vec3 direction = rotation * Direction::kForward;
        const glm::vec3 up = Direction::kUp;

        return CameraLocation{ position, direction, up };
    }

    static CameraProjection RetrieveCameraProjection(const tinygltf::Camera& camera)
    {
        if (camera.type == "perspective")
        {
            const tinygltf::PerspectiveCamera& perspectiveCamera = camera.perspective;

            return CameraProjection{
                static_cast<float>(perspectiveCamera.yfov),
                static_cast<float>(perspectiveCamera.aspectRatio), 1.0f,
                static_cast<float>(perspectiveCamera.znear),
                static_cast<float>(perspectiveCamera.zfar),
            };
        }

        if (camera.type == "orthographic")
        {
            const tinygltf::OrthographicCamera& orthographicCamera = camera.orthographic;

            return CameraProjection{
                0.0f,
                static_cast<float>(orthographicCamera.xmag),
                static_cast<float>(orthographicCamera.ymag),
                static_cast<float>(orthographicCamera.znear),
                static_cast<float>(orthographicCamera.zfar),
            };
        }

        return Config::DefaultCamera::kProjection;
    }
}

class SceneLoader
{
public:
    SceneLoader(Scene& scene_, const Filepath& path)
        : scene(scene_)
    {
        LoadModel(path);

        AddTextureStorageComponent();

        AddMaterialStorageComponent();

        AddGeometryStorageComponent();

        AddRayTracingStorageComponent();

        AddNodes();
    }

private:
    Scene& scene;

    tinygltf::Model model;

    void LoadModel(const Filepath& path)
    {
        tinygltf::TinyGLTF loader;

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
    }

    void AddTextureStorageComponent() const
    {
        auto& tsc = scene.ctx().emplace<TextureStorageComponent>();

        tsc.images = Details::RetrieveImages(model);
        tsc.samplers = Details::RetrieveSamplers(model);

        tsc.textures.reserve(model.textures.size());

        for (const auto& texture : model.textures)
        {
            Assert(texture.source >= 0);

            const vk::ImageView view = tsc.images[texture.source].view;

            vk::Sampler sampler = RenderContext::defaultSampler;
            if (texture.sampler >= 0)
            {
                sampler = tsc.samplers[texture.sampler];
            }

            tsc.textures.emplace_back(view, sampler);
        }
    }

    void AddMaterialStorageComponent() const
    {
        auto& msc = scene.ctx().emplace<MaterialStorageComponent>();

        msc.materials.reserve(model.materials.size());

        for (const auto& material : model.materials)
        {
            msc.materials.push_back(Details::RetrieveMaterial(material));
        }
    }

    void AddGeometryStorageComponent() const
    {
        auto& gsc = scene.ctx().emplace<GeometryStorageComponent>();

        gsc.primitives.reserve(model.meshes.size());

        for (const auto& mesh : model.meshes)
        {
            for (const auto& primitive : mesh.primitives)
            {
                gsc.primitives.push_back(Details::RetrievePrimitive(model, primitive));
            }
        }
    }

    void AddRayTracingStorageComponent() const
    {
        if constexpr (Config::kRayTracingEnabled)
        {
            auto& rtsc = scene.ctx().emplace<RayTracingStorageComponent>();

            rtsc.indexBuffers.reserve(model.meshes.size());
            rtsc.vertexBuffers.reserve(model.meshes.size());

            rtsc.blases.reserve(model.meshes.size());

            for (const auto& mesh : model.meshes)
            {
                for (const auto& primitive : mesh.primitives)
                {
                    rtsc.indexBuffers.push_back(Details::CreateRayTracingIndexBuffer(model, primitive));
                    rtsc.vertexBuffers.push_back(Details::CreateRayTracingVertexBuffer(model, primitive));

                    rtsc.blases.push_back(Details::GenerateBlas(model, primitive));
                }
            }
        }
    }

    void AddNodes() const
    {
        Details::EnumerateNodes(model, [&](const tinygltf::Node& node, entt::entity parent)
            {
                const entt::entity entity = scene.create();

                AddHierarchyComponent(entity, parent);

                AddTransformComponent(entity, node);

                if (node.mesh >= 0)
                {
                    AddRenderComponent(entity, node);
                }

                if (node.camera >= 0)
                {
                    AddCameraComponent(entity, node);
                }

                if (node.extras.Has("environment"))
                {
                    AddEnvironmentComponent(entity, node);
                }
                
                if (node.extras.Has("scene"))
                {
                    AddScene(entity, node);
                }

                return entity;
            });
    }

    void AddHierarchyComponent(entt::entity entity, entt::entity parent) const
    {
        auto& hc = scene.emplace<HierarchyComponent>(entity);

        hc.parent = parent;

        auto& parentHc = scene.get<HierarchyComponent>(entity);

        parentHc.children.push_back(entity);
    }

    void AddTransformComponent(entt::entity entity, const tinygltf::Node& node) const
    {
        auto& tc = scene.emplace<TransformComponent>(entity);

        tc.localTransform = Details::RetrieveTransform(node);

        ComponentHelpers::AccumulateTransform(scene, entity);
    }

    void AddRenderComponent(entt::entity entity, const tinygltf::Node& node) const
    {
        auto& rc = scene.emplace<RenderComponent>(entity);

        const tinygltf::Mesh& mesh = model.meshes[node.mesh];

        size_t meshOffset = 0;
        for (int32_t i = 0; i < node.mesh; ++i)
        {
            meshOffset += model.meshes[i].primitives.size();
        }

        rc.renderObjects.resize(mesh.primitives.size());

        for (size_t i = 0; i < mesh.primitives.size(); ++i)
        {
            const tinygltf::Primitive& primitive = mesh.primitives[i];

            Assert(primitive.material >= 0);

            rc.renderObjects[i].primitive = static_cast<uint32_t>(meshOffset + i);
            rc.renderObjects[i].material = static_cast<uint32_t>(primitive.material);
        }
    }

    void AddCameraComponent(entt::entity entity, const tinygltf::Node& node) const
    {
        auto& cc = scene.emplace<CameraComponent>(entity);

        const tinygltf::Camera& camera = model.cameras[node.camera];

        cc.location = Details::RetrieveCameraLocation(node);
        cc.projection = Details::RetrieveCameraProjection(camera);

        cc.viewMatrix = CameraHelpers::CalculateViewMatrix(cc.location);
        cc.projMatrix = CameraHelpers::CalculateProjMatrix(cc.projection);

        if (!scene.ctx().contains<CameraComponent&>())
        {
            scene.ctx().emplace<CameraComponent&>(cc);
        }
    }

    void AddEnvironmentComponent(entt::entity entity, const tinygltf::Node& node) const
    {
        auto& ec = scene.emplace<EnvironmentComponent>(entity);

        const tinygltf::Value& environment = node.extras.Get("environment");

        const Filepath panoramaPath(environment.Get("panoramaPath").Get<std::string>());

        ec = EnvironmentHelpers::LoadEnvironment(panoramaPath);
        
        if (!scene.ctx().contains<EnvironmentComponent&>())
        {
            scene.ctx().emplace<EnvironmentComponent&>(ec);
        }
    }

    void AddScene(entt::entity entity, const tinygltf::Node& node) const
    {
        const Filepath scenePath(node.extras.Get("scene").Get("path").Get<std::string>());

        scene.AddScene(Scene(scenePath), entity);
    }
};

void SceneHelpers::LoadScene(Scene& scene, const Filepath& path)
{
    SceneLoader sceneLoader(scene, path);
}
