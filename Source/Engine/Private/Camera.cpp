#include "Engine/Camera.hpp"

namespace Details
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
    viewMatrix = Details::CalculateViewMatrix(description);
    projectionMatrix = Details::CalculateProjectionMatrix(description);
}

void Camera::SetPosition(const glm::vec3 &position)
{
    description.position = position;
    viewMatrix = Details::CalculateViewMatrix(description);
}

void Camera::SetDirection(const glm::vec3 &direction)
{
    description.target = description.position + direction;
    viewMatrix = Details::CalculateViewMatrix(description);
}

void Camera::SetTarget(const glm::vec3 &target)
{
    description.target = target;
    viewMatrix = Details::CalculateViewMatrix(description);
}

void Camera::SetUp(const glm::vec3 &up)
{
    description.up = up;
    viewMatrix = Details::CalculateViewMatrix(description);
}

void Camera::SetFov(float yFov)
{
    description.xFov = yFov;
    projectionMatrix = Details::CalculateProjectionMatrix(description);
}

void Camera::SetAspect(float aspect)
{
    description.aspect = aspect;
    projectionMatrix = Details::CalculateProjectionMatrix(description);
}

void Camera::SetZNear(float zNear)
{
    description.zNear = zNear;
    projectionMatrix = Details::CalculateProjectionMatrix(description);
}

void Camera::SetZFar(float zFar)
{
    description.zFar = zFar;
    projectionMatrix = Details::CalculateProjectionMatrix(description);
}
