#include <iostream>
#include <Trex/Atlas.hpp>

#include "geometrygenerator.h"
#include "params.h"
#include "renderer/renderer.h"
#include "image.h"
#include "parser/obj.hpp"
#include "objAdapter.h"

namespace {
void showHelp() {
    std::cout << "\
usage: previewrenderer [ht:o:f:c:i:#:ls]\n\
\n\
    Parameters:\n\
    -h     Show this help message\n\
\n\
    -o     Path to OBJ file\n\
    -t     Path to texture file\n\
    -f     Path to font file\n\
\n\
    -c     Camera settings (format: -c<param><value>)\n\
    p - pitch (degrees)\n\
    y - yaw (degrees)\n\
    d - distance from object\n\
    h - height offset\n\
    f - field of view (degrees)\n\
\n\
    -i     Image settings (format: -i<param><value>)\n\
    w - width (pixels)\n\
    h - height (pixels)\n\
\n\
    -#     Background color (hex ARGB, e.g. -#050505FF)\n\
\n\
    -l     Draw bounding lines\n\
    -s     Draw mesh sizes\n\
\n\
    Example: objrender -o ./fox.obj -t ./foxTexture.png -f /usr/local/share/fonts/h/helvetica_regular.otf -cp35 -cy145 -ch0.40 -cd2.8 -iw2048 -ih2048 -#050505FF -sl"
<< std::endl;
}
}

int main(int argc, char* argv[]) {
    try {
        const Params params(argc, argv);

        if (argc == 1 || params.helpNeeded) {
            showHelp();
            return 0;
        }

        Renderer::UBOCamera camera(
            params.cameraYaw,
            params.cameraPitch,
            params.cameraDistance,
            params.cameraHeight,
            params.cameraFOV,
            (float)params.imageWidth / (float)params.imageHeight
        );

        auto object = Parser::Obj::readFile<ObjectAdapter>(params.objPath);

        const auto aabb = GeometryGenerator::getAABB(object.vertices);
        const auto positioning = GeometryGenerator::getPositioning(aabb, camera);

        const auto lines = GeometryGenerator::generateLines(aabb, positioning, 0.01f);

        Renderer::Params renderParams{
            .renderWidth = static_cast<uint32_t>(params.imageWidth),
            .renderHeight = static_cast<uint32_t>(params.imageHeight),
            .backroundColor = params.backgroundColor,

            .cameraPitch = params.cameraPitch,
            .cameraYaw = params.cameraYaw,
            .cameraHeight = params.cameraHeight,
            .cameraDistance = params.cameraDistance,
            .cameraFOV = params.cameraFOV,

            .objectVertices = object.vertices,
            .drawTexture = object.hasTexture && !params.texturePath.empty(),
            .shading = object.hasNormals,

            .linesVertices = lines,
            .drawLines = params.drawLines,
        };

        Image texture{};
        if (!params.texturePath.empty()) {
            texture = Image(params.texturePath);

            renderParams.objectTexture = texture.getData();
            renderParams.objectTextureWidth = static_cast<uint32_t>(texture.getWidth());
        }

        std::vector<Renderer::Vertex> text;
        std::vector<std::uint8_t> textTexture;
        if (!params.fontPath.empty()) {
            Trex::Atlas atlas(params.fontPath, std::min(params.imageWidth, params.imageHeight) / 6.0f, Trex::Charset::Ascii(), Trex::RenderMode::COLOR);
            Trex::Atlas::Bitmap bitmap = atlas.GetBitmap();

            textTexture = bitmap.Data();

            text = GeometryGenerator::generateText(aabb, positioning, camera, atlas, 0.03f);

            renderParams.textVertices = text;
            renderParams.drawSizes = params.drawSizes && !params.fontPath.empty();
            renderParams.textTexture = textTexture;
            renderParams.textTextureWidth = static_cast<uint32_t>(bitmap.Width());
        }

        Renderer::Instance renderer(renderParams);
        renderer.render();

        auto result = renderer.renderedImage();
        Image output(result, params.imageWidth, params.imageHeight);
        output.save("./output.png");

    }
    catch (std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknow exception" << std::endl;
    }

    return 0;
}
