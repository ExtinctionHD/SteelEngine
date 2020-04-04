#pragma once

struct CameraDescription
{
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 up;
    float fov;
    float aspect;
    float zNear;
    float zFar;
};

class Camera
{
public:
    Camera(const CameraDescription &description_);

    const CameraDescription &GetDescription() const { return description; }

    void SetPosition(const glm::vec3 &position);

    void SetDirection(const glm::vec3 &direction);

    void SetTarget(const glm::vec3 &target);

    void SetUp(const glm::vec3 &up);

    void SetFov(float fov);

    void SetAspect(float aspect);

    void SetZNear(float zNear);

    void SetZFar(float zFar);

    const glm::mat4 &GetViewMatrix() const { return viewMatrix; }

    const glm::mat4 &GetProjectionMatrix() const { return projectionMatrix; }

private:
    CameraDescription description;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};
