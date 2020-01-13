#pragma once

struct DescriptorProperties
{
    vk::DescriptorType type;
    vk::ShaderStageFlags stages;
    uint32_t count = 1;
};