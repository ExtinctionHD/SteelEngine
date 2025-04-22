#pragma once

class Scene;
class Transform;

// TODO move to Components.hpp
struct CameraComponent
{
    float yFov;
    float width;
    float height;
    float zNear;
    float zFar;

    glm::mat4 GetProjMatrix() const;
};
