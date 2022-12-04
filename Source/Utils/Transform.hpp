#pragma once

#include "Utils/Helpers.hpp"

class Scene;

class Transform
{
public:
    static const Transform kIdentity;

    Transform() = default;

    explicit Transform(const glm::mat4& matrix_);
    explicit Transform(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale);

    const glm::mat4& GetMatrix() const;

    glm::vec3 GetTranslation() const;

    glm::quat GetRotation() const;

    glm::vec3 GetScale() const;

    glm::vec3 GetAxis(Axis axis) const;

    glm::vec3 GetScaledAxis(Axis axis) const;

    void SetTranslation(const glm::vec3& translation);

    void SetRotation(const glm::quat& rotation);

    void SetScale(const glm::vec3& scale);

    void operator*=(const Transform& other);

private:
    glm::mat4 matrix = Matrix4::kIdentity;
};

Transform operator*(const Transform& a, const Transform& b);

glm::vec3 operator*(const Transform& t, const glm::vec4& v);
