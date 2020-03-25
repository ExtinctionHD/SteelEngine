#pragma once

#include "Engine/Scene/Node.hpp"

class Filepath;

namespace tinygltf
{
    class Model;
    class Node;
}

class SceneLoader
{
public:
    static std::unique_ptr<Scene> LoadFromFile(const Filepath &path);
};
