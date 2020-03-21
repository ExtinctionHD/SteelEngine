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

struct CameraData
{
    glm::vec4 position;
    glm::vec4 direction;
    glm::vec4 up;
    glm::vec4 right;
    float fovRad;
    float aspect;
    float zNear;
    float zFar;
};

static_assert(sizeof(CameraData) % sizeof(glm::vec4) == 0);

class Camera
{
public:
    Camera(const CameraDescription &description);
    Camera(const CameraData &data_);

    const CameraData &GetData() const { return data; }

    CameraData &AccessData() { return data; }

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

    void UpdateView();

    void UpdateProjection();

private:
    CameraData data;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};
