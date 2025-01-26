#include "Engine/Render/Stages/RenderStage.hpp"

#include "Utils/Assert.hpp"

RenderStage::RenderStage(const SceneRenderContext& context_)
    : context(context_)
{}

void RenderStage::RegisterScene(const Scene* scene_)
{
    RemoveScene();

    scene = scene_;
    Assert(scene);
}
