#pragma once

#include <cstdint>
#include <span>

#include "types.h"



namespace Renderer {

struct Params {
    uint32_t renderWidth = 128;
    uint32_t renderHeight = 128;

    const std::array<float, 4> backroundColor{};

    float cameraPitch = 0.0f;
    float cameraYaw = 0.0f;
    float cameraHeight = 0.0f;
    float cameraDistance = 1.0f;
    float cameraFOV = 90.0f;

    std::span<const Vertex> objectVertices{};
    bool drawTexture = false;
    bool shading = false;
    std::span<const std::uint8_t> objectTexture{};
    std::uint32_t objectTextureWidth = 0;

    std::span<const Vertex> linesVertices{};
    bool drawLines = false;

    std::span<const Vertex> textVertices{};
    bool drawSizes = false;
    std::span<const std::uint8_t> textTexture{};
    std::uint32_t textTextureWidth = 0;
    std::uint32_t textTextureWidth1 = 0;
};

}