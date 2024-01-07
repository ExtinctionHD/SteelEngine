PRAGMA_DISABLE_WARNINGS
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#include <tiny_gltf.h>
PRAGMA_ENABLE_WARNINGS

#include "Engine/Scene/SceneLoader.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Components/AnimationComponent.hpp"
#include "Engine/Scene/Components/EnvironmentComponent.hpp"
#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Primitive.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/AnimationHelpers.hpp"

#include "Utils/Assert.hpp"
#include "Utils/TimeHelpers.hpp"

namespace Details
{
    using EntityMap = std::map<int32_t, entt::entity>;

    using NodeFunc = std::function<entt::entity(int32_t, entt::entity)>;

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
            return {};
        }
    }

    static SamplerDescription GetSamplerDescription(const tinygltf::Sampler& sampler)
    {
        return SamplerDescription{
            .magFilter = GetSamplerFilter(sampler.magFilter),
            .minFilter = GetSamplerFilter(sampler.minFilter),
            .mipmapMode = GetSamplerMipmapMode(sampler.magFilter),
            .addressMode = GetSamplerAddressMode(sampler.wrapS)
        };
    }

    static tinygltf::Value GetObjectFromValue(const tinygltf::Value& value)
    {
        return tinygltf::Value(value.Get<tinygltf::Value::Object>());
    }

    static bool GetBoolFromValue(const tinygltf::Value& value, const std::string& key)
    {
        return value.Has(key) && value.Get(key).Get<bool>();
    }

    static float GetFloatFromValue(const tinygltf::Value& value, const std::string& key, float defaultValue)
    {
        return value.Has(key) ? static_cast<float>(value.Get(key).Get<double>()) : defaultValue;
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
        Assert(values.size() == 4);

        glm::quat quat;
        quat.x = static_cast<float>(values[0]);
        quat.y = static_cast<float>(values[1]);
        quat.z = static_cast<float>(values[2]);
        quat.w = static_cast<float>(values[3]);

        return quat;
    }

    static AnimatedProperty GetAnimatedProperty(const std::string& propertyName)
    {
        if (propertyName == "translation")
        {
            return AnimatedProperty::eTranslation;
        }
        if (propertyName == "rotation")
        {
            return AnimatedProperty::eRotation;
        }

        Assert(propertyName == "scale");

        return AnimatedProperty::eScale;
    }

    static AnimationInterpolation GetAnimationInterpolation(const std::string& interpolationName)
    {
        if (interpolationName == "STEP")
        {
            return AnimationInterpolation::eStep;
        }

        Assert(interpolationName == "LINEAR");

        return AnimationInterpolation::eLinear;
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

    static void EnumerateNodes(const tinygltf::Model& model, const NodeFunc& func)
    {
        using Enumerator = std::function<void(int32_t, entt::entity)>;

        const Enumerator enumerator = [&](int32_t nodeIndex, entt::entity parent)
            {
                const entt::entity entity = func(nodeIndex, parent);

                const tinygltf::Node node = model.nodes[nodeIndex];

                for (const auto& childIndex : node.children)
                {
                    enumerator(childIndex, entity);
                }
            };

        for (const auto& scene : model.scenes)
        {
            for (const auto& nodeIndex : scene.nodes)
            {
                enumerator(nodeIndex, entt::null);
            }
        }
    }

    static int32_t GetConfigNodeIndex(const tinygltf::Model& model)
    {
        for (const auto& scene : model.scenes)
        {
            for (const auto& nodeIndex : scene.nodes)
            {
                const tinygltf::Node& node = model.nodes[nodeIndex];

                if (node.name == "sceneConfig")
                {
                    return nodeIndex;
                }
            }
        }

        return -1;
    }

    static void RemoveConfigNodeReference(tinygltf::Model& model, int32_t nodeIndex)
    {
        for (auto& scene : model.scenes)
        {
            std::erase(scene.nodes, nodeIndex);
        }
    }

    static tinygltf::Value RetrieveConfig(const tinygltf::Model& model, int32_t configNodeIndex)
    {
        using Enumerator = std::function<void(int32_t, tinygltf::Value& value)>;

        const Enumerator enumerator = [&](int32_t nodeIndex, tinygltf::Value& value)
            {
                const tinygltf::Node node = model.nodes[nodeIndex];

                auto& object = value.Get<tinygltf::Value::Object>();

                object.emplace(node.name, GetObjectFromValue(node.extras));

                for (const auto& childIndex : node.children)
                {
                    enumerator(childIndex, object.at(node.name));
                }
            };

        const tinygltf::Node& configNode = model.nodes[configNodeIndex];

        tinygltf::Value config = GetObjectFromValue(configNode.extras);

        for (const auto& childIndex : configNode.children)
        {
            enumerator(childIndex, config);
        }

        return config;
    }

    static std::vector<Texture> LoadTextures(const tinygltf::Model& model, const Filepath& sceneDirectory)
    {
        std::vector<Texture> textures;
        textures.reserve(model.textures.size());

        for (const auto& modelTexture : model.textures)
        {
            Assert(modelTexture.source >= 0);

            const Filepath imagePath(model.images[modelTexture.source].uri);

            Texture texture = TextureCache::GetTexture(sceneDirectory / imagePath);

            if (modelTexture.sampler >= 0)
            {
                const tinygltf::Sampler& modelSampler = model.samplers[modelTexture.sampler];

                texture.sampler = TextureCache::GetSampler(GetSamplerDescription(modelSampler));
            }

            textures.push_back(texture);
        }

        return textures;
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

        if (gltfMaterial.alphaMode == "MASK")
        {
            material.flags |= MaterialFlagBits::eAlphaTest;
        }
        if (gltfMaterial.alphaMode == "BLEND")
        {
            material.flags |= MaterialFlagBits::eAlphaBlend;
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

    template <class T>
    static DataView<T> RetrieveAttribute(const tinygltf::Model& model,
            const tinygltf::Primitive& gltfPrimitive, const std::string& attributeName)
    {
        if (gltfPrimitive.attributes.contains(attributeName))
        {
            const tinygltf::Accessor& accessor = model.accessors[gltfPrimitive.attributes.at(attributeName)];

            Assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            return GetAccessorDataView<T>(model, accessor);
        }

        return {};
    }

    static Primitive RetrievePrimitive(
            const tinygltf::Model& model, const tinygltf::Primitive& gltfPrimitive)
    {
        Assert(gltfPrimitive.indices >= 0);
        const tinygltf::Accessor& indicesAccessor = model.accessors[gltfPrimitive.indices];

        std::vector<uint32_t> indices;

        if (GetIndexType(indicesAccessor.componentType) == vk::IndexType::eUint32)
        {
            indices = GetAccessorDataView<uint32_t>(model, indicesAccessor).GetCopy();
        }
        else
        {
            const DataView<uint16_t> indices16 = GetAccessorDataView<uint16_t>(model, indicesAccessor);

            indices.resize(indices16.size);

            for (size_t i = 0; i < indices16.size; ++i)
            {
                indices[i] = static_cast<uint32_t>(indices16[i]);
            }
        }

        const DataView<glm::vec3> positions = RetrieveAttribute<glm::vec3>(model, gltfPrimitive, "POSITION");
        const DataView<glm::vec3> normals = RetrieveAttribute<glm::vec3>(model, gltfPrimitive, "NORMAL");
        const DataView<glm::vec3> tangents = RetrieveAttribute<glm::vec3>(model, gltfPrimitive, "TANGENT");
        const DataView<glm::vec2> texCoords = RetrieveAttribute<glm::vec2>(model, gltfPrimitive, "TEXCOORD_0");

        return Primitive(std::move(indices),
                positions.GetCopy(), normals.GetCopy(),
                tangents.GetCopy(), texCoords.GetCopy());
    }

    static Animation RetrieveAnimation(const tinygltf::Model& model,
            const tinygltf::Animation& gltfAnimation, const EntityMap& entityMap)
    {
        Animation animation;
        animation.name = gltfAnimation.name;

        for (const auto& channel : gltfAnimation.channels)
        {
            const tinygltf::AnimationSampler& sampler = gltfAnimation.samplers[channel.sampler];

            AnimationTrack animationTrack;
            animationTrack.target = entityMap.at(channel.target_node);
            animationTrack.property = GetAnimatedProperty(channel.target_path);
            animationTrack.interpolation = GetAnimationInterpolation(sampler.interpolation);

            const tinygltf::Accessor& inputAccessor = model.accessors[sampler.input];
            const tinygltf::Accessor& outputAccessor = model.accessors[sampler.output];

            const DataView<float> timeStamps = GetAccessorDataView<float>(model, inputAccessor);

            const DataView<glm::vec4> quatValues = GetAccessorDataView<glm::vec4>(model, outputAccessor);
            const DataView<glm::vec3> vecValues = GetAccessorDataView<glm::vec3>(model, outputAccessor);

            const bool useQuatValues = animationTrack.property == AnimatedProperty::eRotation;

            for (size_t i = 0; i < timeStamps.size; ++i)
            {
                const glm::vec4 value = useQuatValues ? quatValues[i] : glm::vec4(vecValues[i], 0.0f);

                animationTrack.keyFrames.push_back(AnimationKeyFrame{ timeStamps[i], value });
            }

            animation.duration = std::max(animation.duration, timeStamps.GetLast());

            animation.tracks.push_back(animationTrack);
        }

        return animation;
    }

    static AnimationState RetrieveAnimationState(const tinygltf::Value& config, const std::string& name)
    {
        AnimationState state;

        if (config.Has("animationConfig"))
        {
            if (config.Get("animationConfig").Has(name))
            {
                const tinygltf::Value& animationConfig = config.Get("animationConfig").Get(name);

                state.active = Details::GetBoolFromValue(animationConfig, "active");
                state.looped = Details::GetBoolFromValue(animationConfig, "looped");
                state.reverse = Details::GetBoolFromValue(animationConfig, "reverse");
                state.speed = Details::GetFloatFromValue(animationConfig, "speed", 1.0f);
                state.time = Details::GetFloatFromValue(animationConfig, "time", 0.0f);
            }
        }

        return state;
    }

    static Transform RetrieveTransform(const tinygltf::Node& node)
    {
        Transform transform;

        if (!node.matrix.empty())
        {
            const glm::mat4 matrix = glm::make_mat4(node.matrix.data());

            transform = Transform(matrix);
        }

        if (!node.scale.empty())
        {
            transform.SetScale(GetVec<3>(node.scale));
        }

        if (!node.rotation.empty())
        {
            transform.SetRotation(GetQuaternion(node.rotation));
        }

        if (!node.translation.empty())
        {
            transform.SetTranslation(GetVec<3>(node.translation));
        }

        return transform;
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

SceneLoader::SceneLoader(Scene& scene_, const Filepath& path)
    : scene(scene_)
    , sceneDirectory(path.GetDirectory())
{
    model = std::make_unique<tinygltf::Model>();

    config = std::make_unique<tinygltf::Value>();

    LoadModel(path);

    RetrieveConfig();

    AddTextureStorageComponent();

    AddMaterialStorageComponent();

    AddGeometryStorageComponent();

    const EntityMap entityMap = AddEntities();

    AddAnimationComponent(entityMap);
}

SceneLoader::~SceneLoader() = default;

void SceneLoader::LoadModel(const Filepath& path) const
{
    EASY_FUNCTION()

    const auto imageLoader = [](tinygltf::Image*, const int, std::string*,
            std::string*, int, int, const unsigned char*, int, void*)
        {
            return true;
        };

    tinygltf::TinyGLTF loader;
    loader.SetImageLoader(imageLoader, nullptr);

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

void SceneLoader::RetrieveConfig() const
{
    const int32_t nodeIndex = Details::GetConfigNodeIndex(*model);

    if (nodeIndex >= 0)
    {
        Details::RemoveConfigNodeReference(*model, nodeIndex);

        *config = Details::RetrieveConfig(*model, nodeIndex);
    }
}

void SceneLoader::AddTextureStorageComponent() const
{
    EASY_FUNCTION()

    auto& tsc = scene.ctx().emplace<TextureStorageComponent>();

    tsc.textures = Details::LoadTextures(*model, sceneDirectory);
}

void SceneLoader::AddMaterialStorageComponent() const
{
    EASY_FUNCTION()

    auto& msc = scene.ctx().emplace<MaterialStorageComponent>();

    msc.materials.reserve(model->materials.size());

    for (const auto& material : model->materials)
    {
        msc.materials.push_back(Details::RetrieveMaterial(material));
    }
}

void SceneLoader::AddGeometryStorageComponent() const
{
    EASY_FUNCTION()

    auto& gsc = scene.ctx().emplace<GeometryStorageComponent>();

    gsc.primitives.reserve(model->meshes.size());

    for (const auto& mesh : model->meshes)
    {
        for (const auto& primitive : mesh.primitives)
        {
            gsc.primitives.emplace_back(Details::RetrievePrimitive(*model, primitive));
        }
    }
}

SceneLoader::EntityMap SceneLoader::AddEntities() const
{
    EASY_FUNCTION()

    EntityMap entityMap;

    Details::EnumerateNodes(*model, [&](int32_t nodeIndex, entt::entity parent)
        {
            const tinygltf::Node& node = model->nodes[nodeIndex];

            const entt::entity entity = scene.CreateEntity(parent, Details::RetrieveTransform(node));

            entityMap.emplace(nodeIndex, entity);

            if (!node.name.empty())
            {
                scene.emplace<NameComponent>(entity, node.name);
            }

            if (node.mesh >= 0)
            {
                AddRenderComponent(entity, node);
            }

            if (node.camera >= 0)
            {
                AddCameraComponent(entity, node);
            }

            if (node.extensions.contains("KHR_lights_punctual"))
            {
                AddLightComponent(entity, node);
            }

            if (node.extras.Has("environment"))
            {
                AddEnvironmentComponent(entity, node);
            }

            if (node.extras.Has("scenePrefab"))
            {
                const Filepath scenePath(node.extras.Get("scenePrefab").Get<std::string>());

                scene.EmplaceScenePrefab(Scene(scenePath), entity);
            }

            if (node.extras.Has("sceneInstance"))
            {
                const std::string name = node.extras.Get("sceneInstance").Get<std::string>();

                scene.EmplaceSceneInstance(scene.FindEntity(name), entity);
            }

            if (node.extras.Has("sceneSpawn"))
            {
                const std::string name = node.extras.Get("sceneSpawn").Get<std::string>();

                scene.CreateSceneInstance(scene.FindEntity(name), scene.GetEntityTransform(entity));
            }

            return entity;
        });

    return entityMap;
}

void SceneLoader::AddRenderComponent(entt::entity entity, const tinygltf::Node& node) const
{
    EASY_FUNCTION()

    auto& rc = scene.emplace<RenderComponent>(entity);

    const tinygltf::Mesh& mesh = model->meshes[node.mesh];

    size_t meshOffset = 0;
    for (int32_t i = 0; i < node.mesh; ++i)
    {
        meshOffset += model->meshes[i].primitives.size();
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

void SceneLoader::AddCameraComponent(entt::entity entity, const tinygltf::Node& node) const
{
    EASY_FUNCTION()

    auto& cc = scene.emplace<CameraComponent>(entity);

    const tinygltf::Camera& camera = model->cameras[node.camera];

    cc.location = Details::RetrieveCameraLocation(node);
    cc.projection = Details::RetrieveCameraProjection(camera);

    cc.viewMatrix = CameraHelpers::ComputeViewMatrix(cc.location);
    cc.projMatrix = CameraHelpers::ComputeProjMatrix(cc.projection);

    if (!scene.ctx().contains<CameraComponent&>())
    {
        scene.ctx().emplace<CameraComponent&>(cc);
    }
}

void SceneLoader::AddLightComponent(entt::entity entity, const tinygltf::Node& node) const
{
    EASY_FUNCTION()

    auto& lc = scene.emplace<LightComponent>(entity);

    const int32_t lightIndex = node.extensions.at("KHR_lights_punctual").Get("light").Get<int32_t>();

    Assert(lightIndex >= 0);

    const tinygltf::Light& light = model->lights[lightIndex];

    if (light.type == "directional")
    {
        lc.type = LightType::eDirectional;
    }
    else if (light.type == "point")
    {
        lc.type = LightType::ePoint;
    }
    else
    {
        Assert(false);
    }

    lc.color = Details::GetVec<3>(light.color) * static_cast<float>(light.intensity);
}

void SceneLoader::AddEnvironmentComponent(entt::entity entity, const tinygltf::Node& node) const
{
    EASY_FUNCTION()

    auto& ec = scene.emplace<EnvironmentComponent>(entity);

    const Filepath panoramaPath(node.extras.Get("environment").Get<std::string>());

    ec = EnvironmentHelpers::LoadEnvironment(panoramaPath);

    if (!scene.ctx().contains<EnvironmentComponent&>())
    {
        scene.ctx().emplace<EnvironmentComponent&>(ec);
    }
}

void SceneLoader::AddAnimationComponent(const EntityMap& entityMap) const
{
    auto& ac = scene.ctx().emplace<AnimationComponent>();

    model->animations.reserve(model->animations.size());

    for (const auto& animation : model->animations)
    {
        ac.animations.push_back(Details::RetrieveAnimation(*model, animation, entityMap));

        ac.animations.back().state = Details::RetrieveAnimationState(*config, animation.name);
    }
}
