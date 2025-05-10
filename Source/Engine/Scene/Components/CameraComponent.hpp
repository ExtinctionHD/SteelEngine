#pragma once

class Scene;
class Transform;

struct Frustum
{
    std::array<glm::vec3, 8> corners;
};

Frustum operator*(const Transform& t, const Frustum& f);

struct CameraComponent
{
    float yFov;
    float width;
    float height;
    float zNear;
    float zFar;

    glm::mat4 GetProjMatrix() const;

    Frustum GetLocalFrustum() const;

    Frustum GetFrustum(const Transform& transform) const;
};
