#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <Trex/Atlas.hpp>

#include "renderer/types.h"



namespace GeometryGenerator {

struct AABB {
    glm::vec3 center;
    glm::vec3 halfDimensions;
};

struct Positioning {
    glm::mat4 xTransform;
    glm::mat4 yTransform;
    glm::mat4 zTransform;
};

AABB getAABB(std::span<const Renderer::Vertex> vertices);
Positioning getPositioning(AABB boundaries, Renderer::UBOCamera camera);

std::array<Renderer::Vertex, 6> generateLines(AABB edges, Positioning positioning, float offset);
std::vector<Renderer::Vertex> generateText(AABB edges, Positioning positioning, Renderer::UBOCamera camera, const Trex::Atlas& atlas, float offset);
};
