#pragma once

struct RenderObject
{
    uint32_t mesh = 0;
    uint32_t material = 0;
};

struct RenderComponent
{
    std::vector<RenderObject> renderObjects;
};
