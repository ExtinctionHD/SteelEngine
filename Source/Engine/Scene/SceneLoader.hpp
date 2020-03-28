#pragma once

#include "Engine/Scene/Node.hpp"

class Filepath;

class SceneLoader
{
public:
    static std::unique_ptr<Scene> LoadFromFile(const Filepath &path);
};
