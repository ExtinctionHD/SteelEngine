#pragma once

class Scene;

class ImGuiWidget
{
public:
    virtual ~ImGuiWidget() = default;

    void Build(const Scene* scene, float deltaSeconds);

protected:
    struct Context
    {
        entt::entity selectedEntity = entt::null;
    };

    static Context context;

    ImGuiWidget(const std::string& name_);

    virtual void BuildInternal(const Scene* scene, float deltaSeconds) = 0;

private:
    std::string name;
};
