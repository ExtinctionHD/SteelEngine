#pragma once

struct TransformComponent
{
    static constexpr auto in_place_delete = true;

    static glm::mat4 AccumulateTransform(const TransformComponent& tc);

    TransformComponent* parent = nullptr;
    glm::mat4 localTransform;
    glm::mat4 worldTransform;
};

struct CameraComponent
{

};

struct EnvironmentComponent
{

};

struct LightComponent
{
    
};