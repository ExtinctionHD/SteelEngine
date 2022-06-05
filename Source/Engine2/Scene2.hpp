#pragma once

#include <entt/entity/registry.hpp>

#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"
#include "Shaders/Hybrid/Hybrid.h"
#include "Utils/Flags.hpp"

struct Texture;

class Scene2 : public entt::registry
{
public:
    struct Mesh
    {
        struct Vertex
        {
            static const std::vector<vk::Format> kFormat;

            glm::vec3 position;
            glm::vec3 normal;
            glm::vec3 tangent;
            glm::vec2 texCoord;
        };

        vk::IndexType indexType = vk::IndexType::eUint16;
        uint32_t indexCount = 0;
        vk::Buffer indexBuffer;
        vk::Buffer vertexBuffer;
    };

    enum class MaterialFlagBits
    {
        eAlphaTest,
        eDoubleSided,
        eNormalMapping
    };

    using MaterialFlags = Flags<MaterialFlagBits>;

    struct Material
    {
        MaterialData data;
        MaterialFlags flags;

        static ShaderDefines BuildShaderDefines(MaterialFlags flags)
        {
            ShaderDefines defines;

            if (flags & MaterialFlagBits::eAlphaTest)
            {
                defines.emplace("ALPHA_TEST", 1);
            }
            if (flags & MaterialFlagBits::eDoubleSided)
            {
                defines.emplace("DOUBLE_SIDED", 1);
            }
            if (flags & MaterialFlagBits::eNormalMapping)
            {
                defines.emplace("NORMAL_MAPPING", 1);
            }

            return defines;
        }
    };

    struct MaterialTexture
    {
        vk::ImageView view;
        vk::Sampler sampler;
    };
    
    std::vector<Texture> textures;
    std::vector<vk::Sampler> samplers;
    std::vector<MaterialTexture> materialTextures;
    std::vector<Material> materials;
    std::vector<Mesh> meshes;

    void Load(const Filepath& path);
};

OVERLOAD_LOGIC_OPERATORS(Scene2::MaterialFlags, Scene2::MaterialFlagBits)
