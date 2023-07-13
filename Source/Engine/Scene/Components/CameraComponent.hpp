#pragma once

#include "Engine/EngineHelpers.hpp"

struct CameraLocation
{
    glm::vec3 position = Vector3::kZero;
    glm::vec3 direction = Direction::kForward;
    glm::vec3 up = Direction::kUp;
};

struct CameraProjection
{
    float yFov;
    float width;
    float height;
    float zNear;
    float zFar;
};

struct CameraComponent
{
    static constexpr auto in_place_delete = true;

    CameraLocation location;
    CameraProjection projection;
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
};

namespace CameraHelpers
{
    glm::mat4 ComputeViewMatrix(const CameraLocation& location);

    glm::mat4 ComputeProjMatrix(const CameraProjection& projection);
}
