#pragma once

#include "Utils/DataHelpers.hpp"

struct TransformComponent
{
    static constexpr auto in_place_delete = true;
    TransformComponent* parent = nullptr;
    glm::mat4 transform;
};

struct Geometry
{
    struct DataStorage
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec3> tangents;
        std::vector<glm::vec2> texCoords;

        std::vector<uint32_t> indices;
    };

    DataView<glm::vec3> positions;
    DataView<glm::vec3> normals;
    DataView<glm::vec3> tangents;
    DataView<glm::vec2> texCoords;
    DataView<uint32_t> indices;
    DataStorage dataStorage;
};

struct NMaterial
{
    std::string shader;
    std::vector<std::string> defines;
    std::vector<vk::Format> vertexFormat;

};

struct RenderObject
{
    Geometry geometry;
};

struct RenderComponent
{
    std::vector<RenderObject> renderObjects;
};

struct CameraComponent
{
    
};

struct EnvironmentComponent
{
    
};
