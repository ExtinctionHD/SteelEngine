#pragma once

class Camera
{
public:
    struct Description
    {
        glm::vec3 position;
        glm::vec3 target;
        glm::vec3 up;
        float xFov;
        float aspectRatio;
        float zNear;
        float zFar;
    };

    Camera(const Description& description_);

    const Description& GetDescription() const { return description; }

    void SetPosition(const glm::vec3& position);
    void SetDirection(const glm::vec3& direction);
    void SetTarget(const glm::vec3& target);
    void SetUp(const glm::vec3& up);

    void SetFov(float yFov);
    void SetAspectRatio(float aspectRatio);
    void SetZNear(float zNear);
    void SetZFar(float zFar);

    const glm::mat4& GetViewMatrix() const { return viewMatrix; }
    const glm::mat4& GetProjectionMatrix() const { return projectionMatrix; }

    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

private:
    Description description;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};
