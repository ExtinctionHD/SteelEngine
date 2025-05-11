#pragma once

class Transform;

struct Sphere
{
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 0.0f;

    bool IsValid() const { return radius >= 0.0f; }

    void Add(const glm::vec3& point);

    void Add(const Sphere& other);

    void Translate(const glm::vec3& offset);

    void Scale(float scale);

    void Expand(float value);

    bool Intersect(const Sphere& other) const;

    Sphere GetTransformed(const Transform& transform) const;
};

Sphere operator*(const Transform& t, const Sphere& s);
