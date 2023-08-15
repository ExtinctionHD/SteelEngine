#include <glm/gtx/matrix_decompose.hpp>

#include "Utils/Transform.hpp"

#include "Utils/Helpers.hpp"

const Transform Transform::kIdentity = Transform{};

Transform::Transform(const glm::mat4& matrix_)
    : matrix(matrix_)
{}

Transform::Transform(const glm::vec3& translation)
{
    SetTranslation(translation);
}

Transform::Transform(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
{
    const glm::mat4 scaleMatrix = glm::scale(Matrix4::kIdentity, scale);
    const glm::mat4 rotationMatrix = glm::toMat4(rotation);

    matrix = rotationMatrix * scaleMatrix;

    SetTranslation(translation);
}

const glm::mat4& Transform::GetMatrix() const
{
    return matrix;
}

glm::vec3 Transform::GetTranslation() const
{
    return matrix[3];
}

glm::quat Transform::GetRotation() const
{
    glm::mat3 rotationMatrix;
    rotationMatrix[0] = GetAxis(Axis::eX);
    rotationMatrix[1] = GetAxis(Axis::eY);
    rotationMatrix[2] = GetAxis(Axis::eZ);

    return glm::quat(rotationMatrix);
}

glm::vec3 Transform::GetScale() const
{
    glm::vec3 scale;

    scale.x = glm::length(matrix[0]);
    scale.y = glm::length(matrix[1]);
    scale.z = glm::length(matrix[2]);

    return scale;
}

glm::vec3 Transform::GetAxis(Axis axis) const
{
    return glm::normalize(GetScaledAxis(axis));
}

glm::vec3 Transform::GetScaledAxis(Axis axis) const
{
    return matrix[static_cast<int32_t>(axis)];
}

void Transform::SetTranslation(const glm::vec3& translation)
{
    matrix[3].x = translation.x;
    matrix[3].y = translation.y;
    matrix[3].z = translation.z;
}

void Transform::SetRotation(const glm::quat& rotation)
{
    *this = Transform(GetTranslation(), rotation, GetScale());
}

void Transform::SetScale(const glm::vec3& scale)
{
    *this = Transform(GetTranslation(), GetRotation(), scale);
}

void Transform::operator*=(const Transform& other)
{
    *this = *this * other;
}

Transform operator*(const Transform& a, const Transform& b)
{
    return Transform(b.GetMatrix() * a.GetMatrix());
}

glm::vec3 operator*(const Transform& t, const glm::vec4& v)
{
    return t.GetMatrix() * v;
}
