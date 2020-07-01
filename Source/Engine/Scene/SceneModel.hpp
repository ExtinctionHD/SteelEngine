#pragma once

class Filepath;
class SceneRT;

namespace tinygltf
{
    class Model;
}

class SceneModel
{
public:
    SceneModel(const Filepath &path);
    ~SceneModel();

    std::unique_ptr<SceneRT> CreateSceneRT(const Filepath &environmentPath) const;

private:
    std::unique_ptr<tinygltf::Model> model;
};
