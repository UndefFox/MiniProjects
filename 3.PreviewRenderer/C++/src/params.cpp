#include "params.h"

#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <unistd.h>



Params::Params(int argc, char *argv[]) :
    helpNeeded(false),
    texturePath(),
    objPath(),
    fontPath(),
    cameraPitch(0.0f),
    cameraYaw(0.0f),
    cameraDistance(1.0f),
    cameraHeight(0.0f),
    cameraFOV(90.0f),
    imageWidth(128),
    imageHeight(128),
    backgroundColor{1.0f, 1.0f, 1.0f, 1.0f},
    drawLines(false),
    drawSizes(false)
{
    char opt;
    while((opt = getopt(argc, argv, "ht:o:f:c:i:#:ls")) != -1) {
        switch (opt) {
        case 'h':
            helpNeeded = true; return;
        case 't':
            texturePath = std::string(optarg); break;
        case 'o':
            objPath = std::string(optarg); break;
        case 'f':
            fontPath = std::string(optarg); break;
        case 'c': {
            char param = optarg[0];
            float value = std::strtof(optarg + 1, nullptr);
            switch (param) {
                case 'y': cameraYaw = value; break;
                case 'p': cameraPitch = value; break;
                case 'd': cameraDistance = value; break;
                case 'h': cameraHeight = value; break;
                case 'f': cameraFOV = value; break;
            }
            break;
        }
        case 'i': {
            char param = optarg[0];
            int value = std::strtol(optarg + 1, nullptr, 10);
            switch (param) {
                case 'w': imageWidth = value; break;
                case 'h': imageHeight = value; break;
            }
            break;
        }
        case '#': {
            std::uint32_t hex = std::strtoul(optarg, nullptr, 16);
            backgroundColor[0] = ((hex >> 24) & 0xFF) / 255.0f;
            backgroundColor[1] = ((hex >> 16) & 0xFF) / 255.0f;
            backgroundColor[2] = ((hex >> 8) & 0xFF) / 255.0f;
            backgroundColor[3] = (hex & 0xFF) / 255.0f;
            break;
        }
        case 'l': drawLines = true; break;
        case 's': drawSizes = true; break;
        default:
            throw std::runtime_error("Unknown flag: " + std::to_string(opt));
        }
    }

    if (objPath.empty()) throw std::runtime_error("Object file is required.");
    else if (drawSizes && fontPath.empty()) throw std::runtime_error("Font is required for sizes.");
    else if (imageWidth <= 0 || imageHeight <= 0) throw std::runtime_error("Image dimensions must be > 0.");
    else if (cameraFOV < 10.0f || cameraFOV > 120.0f) throw std::runtime_error("Camera FOV must be within 10-120.");
}

