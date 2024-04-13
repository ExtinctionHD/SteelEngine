#pragma once

class Scene;
struct Animation;

class ImGuiWidget
{
public:
    virtual ~ImGuiWidget() = default;

    void Build(Scene* scene, float deltaSeconds);

protected:
    struct Context
    {
        entt::entity selectedEntity = entt::null;
        Animation* selectedAnimation = nullptr;
    };

    static Context context;

    ImGuiWidget(const std::string& name_);

    virtual void BuildInternal(Scene* scene, float deltaSeconds) = 0;

private:
    std::string name;
};
