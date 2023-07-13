#pragma once

#include "Utils/Transform.hpp"

class TransformComponent
{
public:
    const Transform& GetLocalTransform() const { return worldTransform; }

    const Transform& GetWorldTransform() const { return worldTransform; }

    void SetLocalTransform(const Transform& transform);

    void SetLocalTranslation(const glm::vec3& translation);

    void SetLocalRotation(const glm::quat& rotation);

    void SetLocalScale(const glm::vec3& scale);

private:
    Transform localTransform;
    Transform worldTransform;

    bool dirty = true;

    friend class TransformSystem;
};
