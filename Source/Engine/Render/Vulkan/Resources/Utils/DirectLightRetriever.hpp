#pragma once

class ComputePipeline;
struct Texture;

class DirectLightRetriever
{
public:
    DirectLightRetriever();
    ~DirectLightRetriever();

    glm::vec4 Retrieve(const Texture& panoramaTexture) const;

private:
    std::unique_ptr<ComputePipeline> computePipeline;
};
