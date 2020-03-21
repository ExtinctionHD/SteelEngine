#pragma once

#include "Utils/Helpers.hpp"

namespace Direction
{
    const glm::vec3 kForward = Vector3::kZ;
    const glm::vec3 kBackward = -Vector3::kZ;
    const glm::vec3 kRight = Vector3::kX;
    const glm::vec3 kLeft = -Vector3::kX;
    const glm::vec3 kUp = -Vector3::kY;
    const glm::vec3 kDown = Vector3::kY;
}
