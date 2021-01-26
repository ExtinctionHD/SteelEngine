#pragma once

class Filepath;
class Scene;
class ScenePT;
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

    std::unique_ptr<ScenePT> CreateScenePT() const;

    std::unique_ptr<Camera> CreateCamera() const;

private:
    std::unique_ptr<tinygltf::Model> model;
};
