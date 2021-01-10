#pragma once

class Filepath;
class Scene;
class SceneRT;
class Camera;

namespace tinygltf
{
    class Model;
}

class SceneModel
{
public:
    SceneModel(const Filepath& path);
    ~SceneModel();

    std::unique_ptr<Scene> CreateScene() const;

    std::unique_ptr<SceneRT> CreateSceneRT() const;

    std::unique_ptr<Camera> CreateCamera() const;

private:
    std::unique_ptr<tinygltf::Model> model;
};
