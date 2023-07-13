#include "Engine/Scene/Components/CameraComponent.hpp"

#include "Engine/Config.hpp"

namespace Details
{
    glm::mat4 ComputePerspectiveMatrix(float yFov, float width, float height, float zNear, float zFar)
    {
        const float aspectRatio = width / height;

        glm::mat4 projMatrix = glm::perspective(yFov, aspectRatio, zNear, zFar);

        projMatrix[1][1] = -projMatrix[1][1];

        return projMatrix;
    }

    glm::mat4 ComputeOrthographicMatrix(float width, float height, float zNear, float zFar)
    {
        const float halfWidth = width * 0.5f;
        const float halfHeight = height * 0.5f;

        glm::mat4 projMatrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, zNear, zFar);

        projMatrix[1][1] = -projMatrix[1][1];

        return projMatrix;
    }
}

glm::mat4 CameraHelpers::ComputeViewMatrix(const CameraLocation& location)
{
    return glm::lookAt(location.position, location.position + location.direction, location.up);
}

glm::mat4 CameraHelpers::ComputeProjMatrix(const CameraProjection& projection)
{
    const float zNear = Config::kReverseDepth ? projection.zFar : projection.zNear;
    const float zFar = Config::kReverseDepth ? projection.zNear : projection.zFar;

    if (projection.yFov == 0.0f)
    {
        return Details::ComputeOrthographicMatrix(
                projection.width, projection.height, zNear, zFar);
    }

    return Details::ComputePerspectiveMatrix(
            projection.yFov, projection.width, projection.height, zNear, zFar);
}
