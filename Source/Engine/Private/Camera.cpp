#include "Engine/Camera.hpp"

namespace SCamera
{
    glm::mat4 CalculateViewMatrix(const Camera::Description &description)
    {
        return glm::lookAt(description.position, description.target, description.up);
    }

    glm::mat4 CalculateProjectionMatrix(const Camera::Description &description)
    {
        const float yFov = description.xFov / description.aspect;

        glm::mat4 projectionMatrix = glm::perspective(yFov, description.aspect,
                description.zNear, description.zFar);

        projectionMatrix[1][1] = -projectionMatrix[1][1];

        return projectionMatrix;
    }
}

Camera::Camera(const Description &description_)
    : description(description_)
{
    viewMatrix = SCamera::CalculateViewMatrix(description);
    projectionMatrix = SCamera::CalculateProjectionMatrix(description);
}

void Camera::SetPosition(const glm::vec3 &position)
{
    description.position = position;
    viewMatrix = SCamera::CalculateViewMatrix(description);
}

void Camera::SetDirection(const glm::vec3 &direction)
{
    description.target = description.position + direction;
    viewMatrix = SCamera::CalculateViewMatrix(description);
}

void Camera::SetTarget(const glm::vec3 &target)
{
    description.target = target;
    viewMatrix = SCamera::CalculateViewMatrix(description);
}

void Camera::SetUp(const glm::vec3 &up)
{
    description.up = up;
    viewMatrix = SCamera::CalculateViewMatrix(description);
}

void Camera::SetFov(float yFov)
{
    description.xFov = yFov;
    projectionMatrix = SCamera::CalculateProjectionMatrix(description);
}

void Camera::SetAspect(float aspect)
{
    description.aspect = aspect;
    projectionMatrix = SCamera::CalculateProjectionMatrix(description);
}

void Camera::SetZNear(float zNear)
{
    description.zNear = zNear;
    projectionMatrix = SCamera::CalculateProjectionMatrix(description);
}

void Camera::SetZFar(float zFar)
{
    description.zFar = zFar;
    projectionMatrix = SCamera::CalculateProjectionMatrix(description);
}
