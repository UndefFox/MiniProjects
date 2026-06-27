#pragma once

#include "renderer/types.h"
#include <vector>


struct Object {
    std::vector<Renderer::Vertex> vertices;
    bool hasNormals:1 = false;
    bool hasTexture:1 = false;
};

struct ObjectAdapter {
    Object result;

    std::vector<std::array<float, 3>> vertices;
    std::vector<std::array<float, 3>> normals;
    std::vector<std::array<float, 2>> textureCords;

    void nextV(std::array<float, 3> pos);
    void nextVN(std::array<float, 3> normal);
    void nextVT(std::array<float, 2> cord);
    void nextF(std::array<std::array<int, 3>, 3> ids);
};
