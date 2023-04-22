#pragma once

class PipelineBase
{
public:
    vk::Pipeline Get() const { return pipeline; }

    vk::PipelineLayout GetLayout() const { return layout; }

protected:
    PipelineBase(vk::Pipeline pipeline_, vk::PipelineLayout layout_);

private:
    vk::Pipeline pipeline;

    vk::PipelineLayout layout;
};
