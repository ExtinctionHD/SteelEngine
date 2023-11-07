#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#include <tiny_gltf.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "Engine/Engine.hpp"
#include "Engine/Scene/SceneLoader.hpp"

#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/AnimationHelpers.hpp"
#include "Engine/Scene/Components/AnimationComponent.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Components/EnvironmentComponent.hpp"
#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Primitive.hpp"
#include "Engine/Scene/Scene.hpp"

#include "Utils/Assert.hpp"
#include "Utils/TimeHelpers.hpp"

namespace Details
{
    using NodeFunc = std::function<entt::entity(const tinygltf::Node&, entt::entity, int)>;

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
            return {};
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
            return {};
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
        Assert(values.size() == 4);

        glm::quat quat;
        quat.x = static_cast<float>(values[0]);
        quat.y = static_cast<float>(values[1]);
        quat.z = static_cast<float>(values[2]);
        quat.w = static_cast<float>(values[3]);

        return quat;
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
    static DataView<T> GetAccessorDataView(const tinygltf::Model& model, const tinygltf::Accessor& accessor)
    {
        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
        Assert(bufferView.byteStride == 0 || bufferView.byteStride == GetAccessorValueSize(accessor));

        const size_t offset = bufferView.byteOffset + accessor.byteOffset;
        const T* data = reinterpret_cast<const T*>(model.buffers[bufferView.buffer].data.data() + offset);

        return DataView<T>(data, accessor.count);
    }

    static void EnumerateNodes(const tinygltf::Model& model, const NodeFunc& func)
    {
        using Enumerator = std::function<void(const tinygltf::Node&, entt::entity, int)>;

        const Enumerator enumerator = [&](const tinygltf::Node& node, entt::entity parent, int gltfNodeIndex)
        {
            const entt::entity entity = func(node, parent, gltfNodeIndex);

            for (const auto& childIndex : node.children)
            {
                const tinygltf::Node& child = model.nodes[childIndex];

                enumerator(child, entity, childIndex);
            }
        };

        for (const auto& scene : model.scenes)
        {
            for (const auto& nodeIndex : scene.nodes)
            {
                const tinygltf::Node& node = model.nodes[nodeIndex];

                enumerator(node, entt::null, nodeIndex);
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
                0.0f,
                std::numeric_limits<float>::max(),
                false,
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
    static DataView<T> RetrieveAttribute(
            const tinygltf::Model& model, const tinygltf::Primitive& gltfPrimitive, const std::string& attributeName)
    {
        if (gltfPrimitive.attributes.contains(attributeName))
        {
            const tinygltf::Accessor& accessor = model.accessors[gltfPrimitive.attributes.at(attributeName)];

            Assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            return GetAccessorDataView<T>(model, accessor);
        }

        return {};
    }

    static Primitive RetrievePrimitive(const tinygltf::Model& model, const tinygltf::Primitive& gltfPrimitive)
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

        return Primitive(
                std::move(indices), positions.GetCopy(), normals.GetCopy(), tangents.GetCopy(), texCoords.GetCopy());
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
                static_cast<float>(perspectiveCamera.aspectRatio),
                1.0f,
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

        return Config::camera.kProjection;
    }

    static void ParseKeyframeTrack(
            tinygltf::Model* model, const tinygltf::AnimationSampler& sampler, KeyFrameAnimationTrack& track)
    {
        track.uid = AnimationHelpers::GenerateTrackUid();

        // //////////////// keyframe interpolation ///////////////
        track.interpolation = AnimationHelpers::ParseInterpolationType(sampler.interpolation);

        // //////////////////// keyframe times ///////////////////
        const tinygltf::Accessor& keyFrameTimesAccessor = model->accessors[sampler.input];

        Assert(keyFrameTimesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

        const DataView<float>& keyFrameTimesView = Details::GetAccessorDataView<float>(*model, keyFrameTimesAccessor);
        track.keyFrameTimes.resize(keyFrameTimesView.size);
        for (size_t i = 0; i < keyFrameTimesView.size; ++i)
        {
            track.keyFrameTimes[i] = keyFrameTimesView.data[i];
        }

        // //////////////////// keyframe values ///////////////////
        const tinygltf::Accessor& keyValuesAccessor = model->accessors[sampler.output];

        Assert(keyValuesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        if (keyValuesAccessor.type == TINYGLTF_TYPE_VEC4)
        {
            const DataView<glm::vec4>& valuesVec4View =
                    Details::GetAccessorDataView<glm::vec4>(*model, keyValuesAccessor);
            track.values.resize(valuesVec4View.size);
            for (size_t i = 0; i < valuesVec4View.size; ++i)
            {
                track.values[i] = valuesVec4View.data[i];
            }
        }
        else if (keyValuesAccessor.type == TINYGLTF_TYPE_VEC3)
        {
            const DataView<glm::vec3>& valuesVec3View =
                    Details::GetAccessorDataView<glm::vec3>(*model, keyValuesAccessor);
            track.values.resize(valuesVec3View.size);
            for (size_t i = 0; i < valuesVec3View.size; ++i)
            {
                track.values[i] = glm::vec4(valuesVec3View.data[i], 0.0f);
            }
        }
        else
        {
            Assert(false); // << "Unknown gltf keyframe animation value type"
        }
    }

    static bool IsAnimatedEntity(const AnimationParseInfo& animationParseInfo, int gltfNodeIndex)
    {
        return animationParseInfo.animatedNodesIndices.find(gltfNodeIndex) !=
                animationParseInfo.animatedNodesIndices.end();
    }
}

SceneLoader::SceneLoader(Scene& scene_, const Filepath& path) : scene(scene_)
{
    model = std::make_unique<tinygltf::Model>();

    LoadModel(path);

    AddTextureStorageComponent();

    AddMaterialStorageComponent();

    AddGeometryStorageComponent();

    AnimationParseInfo animParseInfo = AddAnimationStorage();

    AddEntities(animParseInfo);

    FinalizeAnimationsSetup(animParseInfo);
}

SceneLoader::~SceneLoader() = default;

void SceneLoader::LoadModel(const Filepath& path) const
{
    EASY_FUNCTION()

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

void SceneLoader::AddTextureStorageComponent() const
{
    EASY_FUNCTION()

    auto& tsc = scene.ctx().emplace<TextureStorageComponent>();

    tsc.textures = Details::RetrieveImages(*model);
    tsc.samplers = Details::RetrieveSamplers(*model);

    tsc.textures.reserve(model->textures.size());

    for (const auto& texture : model->textures)
    {
        Assert(texture.source >= 0);

        const vk::ImageView view = tsc.textures[texture.source].view;

        vk::Sampler sampler = RenderContext::defaultSampler;
        if (texture.sampler >= 0)
        {
            sampler = tsc.samplers[texture.sampler];
        }

        TextureSampler textureSampler{ view, sampler };
        tsc.textureSamplers.emplace_back(std::move(textureSampler));
    }
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

AnimationParseInfo SceneLoader::AddAnimationStorage()
{
    AnimationParseInfo animPI;
    auto& asc = scene.ctx().emplace<AnimationStorageComponent>();

    asc.animationTracks.reserve(model->animations.size()); // should be multiplied by sampler count in each but still..

    for (const tinygltf::Animation& animInfo : model->animations)
    {
        for (const tinygltf::AnimationSampler& sampler : animInfo.samplers)
        {
            KeyFrameAnimationTrack& track = asc.animationTracks.emplace_back();
            Details::ParseKeyframeTrack(model.get(), sampler, track);
        }

        for (const tinygltf::AnimationChannel& channel : animInfo.channels)
        {
            animPI.animatedNodesIndices.insert(channel.target_node);
        }

        Animation anim;

        anim.uid = AnimationHelpers::GenerateAnimationUid();
        anim.name = animInfo.name;

        if (Config::app.autoplayAnims.find(anim.name) != Config::app.autoplayAnims.end())
        {
            anim.StartLooped();
        }

        auto it = Config::app.animPlaySpeeds.find(anim.name);
        if (it != Config::app.animPlaySpeeds.end())
        {
            anim.playbackSpeed = it->second;
        }

        AnimationParseInfo::AnimParseMapping animParseMapping{ animInfo, std::move(anim) };
        animPI.animationsMapping.emplace_back(std::move(animParseMapping));
    }

    return animPI;
}

void SceneLoader::FinalizeAnimationsSetup(const AnimationParseInfo& animPI)
{
    for (const auto& animParseInfoEntry : animPI.animationsMapping)
    {
        const Animation& anim = animParseInfoEntry.anim;

        entt::entity commonParent = SceneHelpers::FindCommonParent(scene, anim.animatedEntities);
        if (commonParent == entt::null)
        {
            Assert(false);
            continue;
        }

        AnimationControlComponent& animationControlComponent =
                scene.get_or_emplace<AnimationControlComponent>(commonParent);
        animationControlComponent.animations.emplace_back(anim);
    }
}

void SceneLoader::AddEntities(AnimationParseInfo& animationParseInfo) const
{
    EASY_FUNCTION()

    Details::EnumerateNodes(*model,
            [&](const tinygltf::Node& node, entt::entity parent, int gltfNodeIndex)
            {
                const entt::entity entity = scene.CreateEntity(parent, Details::RetrieveTransform(node));

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

                if (Details::IsAnimatedEntity(animationParseInfo, gltfNodeIndex))
                {
                    AddAnimationComponent(entity, animationParseInfo, gltfNodeIndex);
                }

                if (node.extras.Has("scene_prefab"))
                {
                    const Filepath scenePath(node.extras.Get("scene_prefab").Get<std::string>());

                    scene.EmplaceScenePrefab(Scene(scenePath), entity);
                }

                if (node.extras.Has("scene_instance"))
                {
                    const std::string name = node.extras.Get("scene_instance").Get<std::string>();

                    scene.EmplaceSceneInstance(scene.FindEntity(name), entity);
                }

                if (node.extras.Has("scene_spawn"))
                {
                    const std::string name = node.extras.Get("scene_spawn").Get<std::string>();

                    scene.CreateSceneInstance(scene.FindEntity(name), scene.GetEntityTransform(entity));
                }

                return entity;
            });
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

    const Filepath panoramaPath(node.extras.Get("environment").Get("panorama_path").Get<std::string>());

    ec = EnvironmentHelpers::LoadEnvironment(panoramaPath);

    if (!scene.ctx().contains<EnvironmentComponent&>())
    {
        scene.ctx().emplace<EnvironmentComponent&>(ec);
    }
}

void SceneLoader::AddAnimationComponent(
        entt::entity entity, AnimationParseInfo& animationParseInfo, int gltfNodeIndex) const
{
    AnimationComponent& animationComponent = scene.emplace<AnimationComponent>(entity);

    AnimationStorageComponent& asc = scene.ctx().get<AnimationStorageComponent>();

    for (auto& animMapping : animationParseInfo.animationsMapping)
    {
        const tinygltf::Animation& animInfo = animMapping.gltfAnim;
        Animation& anim = animMapping.anim;

        for (const tinygltf::AnimationChannel& channel : animInfo.channels)
        {
            if (channel.target_node != gltfNodeIndex)
            {
                continue;
            }

            KeyFrameAnimationTrack& track = asc.animationTracks.at(channel.sampler);
            TrackUid trackUidTmp = track.uid;
            if (track.propName.empty() || track.propName == channel.target_path)
            {
                track.propName = channel.target_path; // "rotation", "translation", "scale"
            }
            else
            {
                KeyFrameAnimationTrack& newTrack = asc.animationTracks.emplace_back();
                newTrack = track;
                newTrack.uid = AnimationHelpers::GenerateTrackUid();
                newTrack.propName = channel.target_path;
                trackUidTmp = newTrack.uid;
            }

            anim.animatedEntities.insert(entity);
            animationComponent.animationTracks[anim.uid].push_back(trackUidTmp);
        }
    }
}
