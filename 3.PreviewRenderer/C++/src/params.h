#pragma once

#include <array>
#include <string>



class Params {
public:
    bool helpNeeded;

    std::string objPath;
    std::string fontPath;
    std::string texturePath;

    float cameraPitch;
    float cameraYaw;
    float cameraDistance;
    float cameraHeight;
    float cameraFOV;

    int imageWidth;
    int imageHeight;

    std::array<float, 4> backgroundColor;

    bool drawLines;
    bool drawSizes;

public:
    Params(int argc, char *argv[]);
};
