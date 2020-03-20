#pragma once

struct CameraProperties
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
    Camera(const CameraProperties &properties_);

    const CameraProperties &GetProperties() const { return properties; }

    CameraProperties &AccessProperties() { return properties; }

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

    void Update();

private:
    CameraProperties properties;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};
