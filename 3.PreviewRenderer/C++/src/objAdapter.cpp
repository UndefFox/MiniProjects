#include "objAdapter.h"


void ObjectAdapter::nextV(std::array<float, 3> pos) {
    vertices.emplace_back(std::move(pos));
}

void ObjectAdapter::nextVN(std::array<float, 3> normal) {
    result.hasNormals = true;
    normals.emplace_back(std::move(normal));
}

void ObjectAdapter::nextVT(std::array<float, 2> cord) {
    result.hasTexture = true;
    textureCords.emplace_back(std::move(cord));
}

void ObjectAdapter::nextF(std::array<std::array<int, 3>, 3> ids) {
    for (const auto vert : ids) {
        const auto position = vertices[vert[0] - 1];
        const auto textureCord = result.hasTexture ? textureCords[vert[1] - 1] : std::array<float, 2>{};
        const auto normal = result.hasNormals ? normals[vert[2] - 1] : std::array<float, 3>{};

        result.vertices.emplace_back(Renderer::Vertex{
            .pos = glm::vec3(position[0], position[1], position[2]),
            .normal = glm::vec3(normal[0], normal[1], normal[2]),
            .texCoord = glm::vec2(textureCord[0], textureCord[1])
        });
    }
}
