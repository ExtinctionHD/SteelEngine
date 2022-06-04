#pragma once

namespace Steel
{
    struct TransformComponent
    {
        static constexpr auto in_place_delete = true;
        TransformComponent* parent = nullptr;
        glm::mat4 transform;
    };

    struct CameraComponent
    {

    };

    struct EnvironmentComponent
    {

    };
}