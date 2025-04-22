#include <glm/gtx/matrix_decompose.hpp>

#include "Engine/Scene/Transform.hpp"

#include "Engine/EngineHelpers.hpp"
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

Transform::Transform(const glm::vec3& translation, const glm::vec3& direction, const glm::vec3& up)
{
    const glm::vec3 zAxis = glm::normalize(-direction);
    const glm::vec3 xAxis = glm::normalize(glm::cross(up, zAxis));
    const glm::vec3 yAxis = glm::cross(zAxis, xAxis);

    matrix[0] = glm::vec4(xAxis, 0.0f);
    matrix[1] = glm::vec4(yAxis, 0.0f);
    matrix[2] = glm::vec4(zAxis, 0.0f);

    SetTranslation(translation);
}

glm::mat3 Transform::GetRotationMatrix() const
{
    glm::mat3 rotationMatrix;
    rotationMatrix[0] = GetAxis(Axis::eX);
    rotationMatrix[1] = GetAxis(Axis::eY);
    rotationMatrix[2] = GetAxis(Axis::eZ);

    return rotationMatrix;
}

glm::quat Transform::GetRotation() const
{
    return glm::quat(GetRotationMatrix());
}

glm::vec3 Transform::GetScale() const
{
    glm::vec3 scale;

    scale.x = glm::length(matrix[0]);
    scale.y = glm::length(matrix[1]);
    scale.z = glm::length(matrix[2]);

    return scale;
}

glm::vec3 Transform::GetScaledAxis(Axis axis) const
{
    return matrix[static_cast<int32_t>(axis)];
}

glm::vec3 Transform::GetAxis(Axis axis) const
{
    return glm::normalize(GetScaledAxis(axis));
}

Transform Transform::GetInverse() const
{
    return Transform(glm::inverse(matrix));
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

void Transform::SetDirection(const glm::vec3& direction)
{
    SetRotation(glm::rotation(Direction::kForward, direction));
}

void Transform::SetScale(const glm::vec3& scale)
{
    matrix[0] = glm::normalize(matrix[0]) * scale.x;
    matrix[1] = glm::normalize(matrix[1]) * scale.y;
    matrix[2] = glm::normalize(matrix[2]) * scale.z;
}

void Transform::Translate(const glm::vec3& translation)
{
    matrix[3] += glm::vec4(translation, 0.0f);
}

void Transform::Rotate(const glm::quat& rotation)
{
    matrix = glm::toMat4(rotation) * matrix;
}

void Transform::Scale(const glm::vec3& scale)
{
    matrix = glm::scale(matrix, scale);
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
