#pragma once

#include "Utils/Helpers.hpp"

class Scene;

class Transform
{
public:
    static const Transform kIdentity;

    Transform() = default;

    explicit Transform(const glm::mat4& matrix_);

    explicit Transform(const glm::vec3& translation);

    explicit Transform(const glm::vec3& translation,
            const glm::quat& rotation, const glm::vec3& scale);

    explicit Transform(const glm::vec3& translation,
            const glm::vec3& direction, const glm::vec3& up);

    const glm::mat4& GetMatrix() const { return matrix; }

    glm::mat4 GetLookAtMatrix() const { return glm::inverse(matrix); }

    glm::vec3 GetTranslation() const { return matrix[3]; }

    glm::vec3 GetDirection() const { return GetForward(); }

    glm::mat3 GetRotationMatrix() const;

    glm::quat GetRotation() const;

    glm::vec3 GetScale() const;

    glm::vec3 GetScaledAxis(Axis axis) const;

    glm::vec3 GetAxis(Axis axis) const;

    glm::vec3 GetForward() const { return -GetAxis(Axis::eZ); }

    glm::vec3 GetBackward() const { return GetAxis(Axis::eZ); }

    glm::vec3 GetRight() const { return GetAxis(Axis::eX); }

    glm::vec3 GetLeft() const { return -GetAxis(Axis::eX); }

    glm::vec3 GetDown() const { return -GetAxis(Axis::eZ); }

    glm::vec3 GetUp() const { return GetAxis(Axis::eY); }

    Transform GetInverse() const;

    void SetTranslation(const glm::vec3& translation);

    void SetRotation(const glm::quat& rotation);

    void SetDirection(const glm::vec3& direction);

    void SetScale(const glm::vec3& scale);

    void Translate(const glm::vec3& translation);

    void Rotate(const glm::quat& rotation);

    void Scale(const glm::vec3& scale);

    void operator*=(const Transform& other);

private:
    glm::mat4 matrix = Matrix4::kIdentity;
};

Transform operator*(const Transform& a, const Transform& b);

glm::vec3 operator*(const Transform& t, const glm::vec4& v);
