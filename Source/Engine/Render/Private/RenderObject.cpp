#include "Engine/Render/RenderObject.hpp"

const VertexFormat Vertex::kFormat{
    vk::Format::eR32G32B32Sfloat,
    vk::Format::eR32G32B32Sfloat,
    vk::Format::eR32G32B32Sfloat,
    vk::Format::eR32G32Sfloat,
};

RenderObject::RenderObject(const std::vector<Vertex> &vertices_,
        const std::vector<uint32_t> &indices_,
        const Material &material_)
    : vertices(vertices_)
    , indices(indices_)
    , material(material_)
{}
