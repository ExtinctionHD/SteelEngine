#pragma once

#include "Engine/Scene/Primitive.hpp"

class BasicMeshes
{
public:
    static void Create();
    static void Destroy();

    static const Primitive& Cube() { return cube; }

    static const Primitive& Sphere() { return sphere; }

    static const Primitive& Cylinder() { return cylinder; }

private:
    static Primitive cube;
    static Primitive sphere;
    static Primitive cylinder;
};
