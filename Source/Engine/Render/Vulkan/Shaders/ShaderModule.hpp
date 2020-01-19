#pragma once

struct ShaderModule
{
    vk::ShaderStageFlagBits stage;
    vk::ShaderModule module;
};
