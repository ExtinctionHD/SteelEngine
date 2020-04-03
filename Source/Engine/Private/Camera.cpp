#include "Engine/Camera.hpp"

namespace SCamera
{
    glm::mat4 CalculateViewMatrix(const CameraData &properties)
    {
        const glm::vec3 position(properties.position);
        const glm::vec3 target(properties.position + properties.direction);
        const glm::vec3 up(properties.up);

        return glm::lookAt(position, target, up);
    }

    glm::mat4 CalculateProjectionMatrix(const CameraData &properties)
    {
        glm::mat4 projectionMatrix = glm::perspective(properties.fovRad / properties.aspect,
                properties.aspect, properties.zNear, properties.zFar);

        projectionMatrix[1][1] = -projectionMatrix[1][1];

        return projectionMatrix;
    }
}

Camera::Camera(const CameraDescription &description)
{
    data.position = glm::vec4(description.position, 1.0f);
    data.direction = glm::vec4(description.direction, 0.0f);
    data.up = glm::vec4(description.up, 0.0f);
    data.right = glm::vec4(glm::cross(description.direction, description.up), 0.0f);
    data.fovRad = glm::radians(description.fov);
    data.aspect = description.aspect;
    data.zNear = description.zNear;
    data.zFar = description.zFar;

    viewMatrix = SCamera::CalculateViewMatrix(data);
    projectionMatrix = SCamera::CalculateProjectionMatrix(data);
}

Camera::Camera(const CameraData &data_)
    : data(data_)
{
    viewMatrix = SCamera::CalculateViewMatrix(data);
    projectionMatrix = SCamera::CalculateProjectionMatrix(data);
}

void Camera::SetPosition(const glm::vec3 &position)
{
    data.position = glm::vec4(position, 1.0f);
    viewMatrix = SCamera::CalculateViewMatrix(data);
}

void Camera::SetDirection(const glm::vec3 &direction)
{
    data.direction = glm::vec4(direction, 0.0f);
    data.right = glm::vec4(glm::cross(direction, glm::vec3(data.up)), 0.0f);
    viewMatrix = SCamera::CalculateViewMatrix(data);
}

void Camera::SetTarget(const glm::vec3 &target)
{
    data.direction = glm::vec4(target, 1.0f) - data.position;
    data.right = glm::vec4(glm::cross(glm::vec3(data.direction), glm::vec3(data.up)), 0.0f);
    viewMatrix = SCamera::CalculateViewMatrix(data);
}

void Camera::SetUp(const glm::vec3 &up)
{
    data.up = glm::vec4(up, 0.0f);
    viewMatrix = SCamera::CalculateViewMatrix(data);
}

void Camera::SetFov(float fov)
{
    data.fovRad = glm::radians(fov);
    projectionMatrix = SCamera::CalculateProjectionMatrix(data);
}

void Camera::SetAspect(float aspect)
{
    data.aspect = aspect;
    projectionMatrix = SCamera::CalculateProjectionMatrix(data);
}

void Camera::SetZNear(float zNear)
{
    data.zNear = zNear;
    projectionMatrix = SCamera::CalculateProjectionMatrix(data);
}

void Camera::SetZFar(float zFar)
{
    data.zFar = zFar;
    projectionMatrix = SCamera::CalculateProjectionMatrix(data);
}

void Camera::UpdateView()
{
    data.right = glm::vec4(glm::cross(glm::vec3(data.direction), glm::vec3(data.up)), 0.0f);
    viewMatrix = SCamera::CalculateViewMatrix(data);
}

void Camera::UpdateProjection()
{
    projectionMatrix = SCamera::CalculateProjectionMatrix(data);
}
