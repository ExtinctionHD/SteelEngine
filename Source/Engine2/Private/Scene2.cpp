#include <tiny_gltf.h>

#include "Engine2/Scene2.hpp"

#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine2/Components2.hpp"
#include "Engine2/RenderComponent.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"
#include "Utils/Logger.hpp"

using namespace Steel;

namespace Details
{
    namespace Utils
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
                Assert(false);
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
                Assert(false);
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
                Assert(false);
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
                Assert(false);
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

    using NodeFunctor = std::function<void(const tinygltf::Node&, entt::entity)>;
    
    static void EnumerateNodes(const tinygltf::Model& model, const NodeFunctor& functor)
    {
        const NodeFunctor enumerator = [&](const tinygltf::Node& node, entt::entity parent)
            {
                functor(node, parent);

                for (const auto& childIndex : node.children)
                {
                    const tinygltf::Node& child = model.nodes[childIndex];

                    enumerator(child, parent);
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
    
    static Material GetMaterial(const tinygltf::Material& gltfMaterial)
    {
        Assert(gltfMaterial.pbrMetallicRoughness.baseColorTexture.texCoord == 0);
        Assert(gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.texCoord == 0);
        Assert(gltfMaterial.normalTexture.texCoord == 0);
        Assert(gltfMaterial.occlusionTexture.texCoord == 0);
        Assert(gltfMaterial.emissiveTexture.texCoord == 0);

        Material material;

        material.technique.doubleSide = gltfMaterial.doubleSided;
        material.technique.alphaTest = gltfMaterial.alphaMode != "OPAQUE";

        material.data.baseColorFactor = Utils::GetVec<4>(gltfMaterial.pbrMetallicRoughness.baseColorFactor);
        material.data.emissionFactor = Utils::GetVec<4>(gltfMaterial.emissiveFactor);
        material.data.baseColorTexture = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
        material.data.roughnessMetallicTexture = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
        material.data.normalTexture = gltfMaterial.normalTexture.index;
        material.data.occlusionTexture = gltfMaterial.occlusionTexture.index;
        material.data.emissionTexture = gltfMaterial.emissiveTexture.index;
        material.data.roughnessFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor);
        material.data.metallicFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor);
        material.data.normalScale = static_cast<float>(gltfMaterial.normalTexture.scale);
        material.data.occlusionStrength = static_cast<float>(gltfMaterial.occlusionTexture.strength);
        material.data.alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);

        return material;
    }

    static Geometry GetGeometry(const tinygltf::Model& model, const tinygltf::Primitive& primitive)
    {
        Geometry geometry;

        Assert(primitive.indices >= 0);
        const tinygltf::Accessor& indicesAccessor = model.accessors[primitive.indices];

        geometry.indexType = Utils::GetIndexType(indicesAccessor.componentType);
        geometry.indices = Utils::GetAccessorByteView(model, indicesAccessor);

        Assert(primitive.attributes.contains("POSITION"));
        const tinygltf::Accessor& positionsAccessor = model.accessors[primitive.attributes.at("POSITION")];

        Assert(positionsAccessor.type == TINYGLTF_TYPE_VEC3);
        Assert(positionsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        geometry.positions = Utils::GetAccessorDataView<glm::vec3>(model, positionsAccessor);

        if (primitive.attributes.contains("NORMAL"))
        {
            const tinygltf::Accessor& normalsAccessor = model.accessors[primitive.attributes.at("NORMAL")];

            Assert(normalsAccessor.type == TINYGLTF_TYPE_VEC3);
            Assert(normalsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            geometry.normals = Utils::GetAccessorDataView<glm::vec3>(model, normalsAccessor);
        }

        if (primitive.attributes.contains("TANGENT"))
        {
            const tinygltf::Accessor& tangentsAccessor = model.accessors[primitive.attributes.at("TANGENT")];

            Assert(tangentsAccessor.type == TINYGLTF_TYPE_VEC3);
            Assert(tangentsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            geometry.tangents = Utils::GetAccessorDataView<glm::vec3>(model, tangentsAccessor);
        }

        if (primitive.attributes.contains("TEXCOORD_0"))
        {
            const tinygltf::Accessor& texCoordsAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];

            Assert(texCoordsAccessor.type == TINYGLTF_TYPE_VEC2);
            Assert(texCoordsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            geometry.texCoords = Utils::GetAccessorDataView<glm::vec2>(model, texCoordsAccessor);
        }

        return geometry;
    }

    static std::vector<Texture> CreateTextures(const tinygltf::Model& model)
    {
        std::vector<Texture> textures;
        textures.reserve(model.images.size());

        for (const auto& image : model.images)
        {
            const vk::Format format = Utils::GetFormat(image);
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
                Utils::GetSamplerFilter(sampler.magFilter),
                Utils::GetSamplerFilter(sampler.minFilter),
                Utils::GetSamplerMipmapMode(sampler.magFilter),
                Utils::GetSamplerAddressMode(sampler.wrapS),
                VulkanConfig::kMaxAnisotropy,
                0.0f, std::numeric_limits<float>::max(),
                false
            };

            samplers.push_back(VulkanContext::textureManager->CreateSampler(samplerDescription));
        }

        return samplers;
    }
}

class SceneLoader
{
public:
    SceneLoader(const Filepath& path, Scene2& scene_)
        : scene(scene_)
    {
        LoadModel(path);
        
        LoadTextures();

        LoadMaterials();

        LoadGeometry();

        LoadNodes();

        scene.registry.ctx().emplace<tinygltf::Model>(std::move(model));
    }

private:
    tinygltf::Model model;

    Scene2& scene;

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

    void LoadTextures() const
    {
        scene.textures = Details::CreateTextures(model);
        scene.samplers = Details::CreateSamplers(model);

        scene.materialTextures.reserve(model.textures.size());

        for (const tinygltf::Texture& texture : model.textures)
        {
            Assert(texture.source >= 0);

            const vk::ImageView view = scene.textures[texture.source].view;

            vk::Sampler sampler = RenderContext::defaultSampler;
            if (texture.sampler >= 0)
            {
                sampler = scene.samplers[texture.sampler];
            }

            scene.materialTextures.emplace_back(view, sampler);
        }
    }

    void LoadMaterials() const
    {
        scene.materials.reserve(model.materials.size());

        for (const tinygltf::Material& material : model.materials)
        {
            scene.materials.push_back(Details::GetMaterial(material));
        }
    }

    void LoadGeometry() const
    {
        scene.geometry.reserve(model.meshes.size());

        for (const tinygltf::Mesh& mesh : model.meshes)
        {
            Assert(mesh.primitives.size() == 1);

            const tinygltf::Primitive& primitive = mesh.primitives.front();

            scene.geometry.push_back(Details::GetGeometry(model, primitive));
        }
    }

    void LoadNodes() const
    {
        Details::EnumerateNodes(model, [&](const tinygltf::Node& node, entt::entity parentEntity)
            {
                const entt::entity entity = scene.registry.create();

                AddTransformComponent(entity, parentEntity, node);

                if (node.mesh >= 0)
                {
                    AddRenderComponent(entity, node.mesh);
                }

                if (node.camera >= 0)
                {
                    scene.registry.emplace<CameraComponent>(entity);
                }

                if (node.extras.Has("environment"))
                {
                    scene.registry.emplace<EnvironmentComponent>(entity);
                }
            });
    }

    void AddTransformComponent(entt::entity entity, entt::entity parent, const tinygltf::Node& node) const
    {
        TransformComponent& tc = scene.registry.emplace<TransformComponent>(entity);

        if (parent != entt::null)
        {
            tc.parent = &scene.registry.get<TransformComponent>(entity);
        }

        tc.transform = Details::Utils::GetTransform(node);
    }

    void AddRenderComponent(entt::entity entity, int32_t meshIndex) const
    {
        RenderComponent& rc = scene.registry.emplace<RenderComponent>(entity);

        rc.geometry = static_cast<uint32_t>(meshIndex);

        const tinygltf::Mesh& mesh = model.meshes[meshIndex];

        Assert(mesh.primitives.size() == 1);

        const tinygltf::Primitive& primitive = mesh.primitives.front();

        Assert(primitive.material >= 0);
        
        rc.material = static_cast<uint32_t>(primitive.material);
    }
};

void Scene2::Load(const Filepath& path)
{
    SceneLoader loader(path, *this);
}
