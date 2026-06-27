#include "geometrygenerator.h"

#include <array>
#include <format>

#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vector_angle.hpp>
#include <Trex/TextShaper.hpp>



namespace {
glm::vec3 toScreen(const glm::vec3& worldPos, const glm::mat4& view) {
    glm::vec4 clipSpace = view * glm::vec4(worldPos, 1.0f);
    return glm::vec3(clipSpace) / clipSpace.w;
}

}

GeometryGenerator::AABB GeometryGenerator::getAABB(std::span<const Renderer::Vertex> vertices) {
    Renderer::Vertex positiveCorner = vertices[0], negativeCorner = vertices[0];

    for (const auto v : vertices) {
        if (v.pos.x > positiveCorner.pos.x) positiveCorner.pos.x = v.pos.x;
        if (v.pos.y > positiveCorner.pos.y) positiveCorner.pos.y = v.pos.y;
        if (v.pos.z > positiveCorner.pos.z) positiveCorner.pos.z = v.pos.z;

        if (v.pos.x < negativeCorner.pos.x) negativeCorner.pos.x = v.pos.x;
        if (v.pos.y < negativeCorner.pos.y) negativeCorner.pos.y = v.pos.y;
        if (v.pos.z < negativeCorner.pos.z) negativeCorner.pos.z = v.pos.z;
    }

    return AABB{
        .center = (positiveCorner.pos + negativeCorner.pos) / 2.0f,
        .halfDimensions = (positiveCorner.pos - negativeCorner.pos) / 2.0f
    };
}

GeometryGenerator::Positioning GeometryGenerator::getPositioning(AABB boundaries, Renderer::UBOCamera camera) {
    const glm::mat4 viewProj = camera.proj * camera.view;

    const glm::vec4 corner1 = glm::vec4(boundaries.halfDimensions, 1.0f);
    const glm::vec4 corner2 = corner1 * glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f);
    const glm::vec4 corner3 = corner1 * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);
    const glm::vec4 corner4 = corner1 * glm::vec4(1.0f, -1.0f, -1.0f, 1.0f);

    auto sortEdgesByDistance = [=](const glm::vec4 projection) -> std::array<glm::vec4, 4> {
        std::array<glm::vec4, 4> result{
            corner1 * projection,
            corner2 * projection,
            corner3 * projection,
            corner4 * projection,
        };

        std::sort(result.begin(), result.end(), [=](const auto& a, const auto& b){ return (viewProj * a).z < (viewProj * b).z; });

        return result;
    };

    const glm::vec3 XZCenter = sortEdgesByDistance(glm::vec4(1.0f, 0.0f, 1.0f, 1.0f))[1] + glm::vec4(boundaries.center, 0.0f);
    const glm::vec3 YZCenter = sortEdgesByDistance(glm::vec4(0.0f, 1.0f, 1.0f, 1.0f))[1] + glm::vec4(boundaries.center, 0.0f);
    const glm::vec3 XYCenter = sortEdgesByDistance(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f))[1] + glm::vec4(boundaries.center, 0.0f);

    enum class Axis {
        X,
        Y,
        Z
    };
    auto findRotation = [=](const glm::vec3 point, Axis paralel) -> glm::vec3 {
        const glm::vec4 p = glm::vec4(point - boundaries.center, 1.0f);

        std::array<glm::vec4, 3> directions{
            glm::vec4(std::signbit(p.x) ? -1.0f : 1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, std::signbit(p.y) ? -1.0f : 1.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, std::signbit(p.z) ? -1.0f : 1.0f, 0.0f)
        };

        const glm::vec4 centerPoint = glm::vec4(point, 1.0f);
        const glm::vec4 xPoint = centerPoint + directions[0];
        const glm::vec4 yPoint = centerPoint + directions[1];
        const glm::vec4 zPoint = centerPoint + directions[2];

        const glm::vec2 centerPointScreen = toScreen(centerPoint, viewProj);
        const glm::vec2 xPointScreen = toScreen(xPoint, viewProj);
        const glm::vec2 yPointScreen = toScreen(yPoint, viewProj);
        const glm::vec2 zPointScreen = toScreen(zPoint, viewProj);

        glm::vec2 paralelEndCordsScreen;

        if (paralel == Axis::X) paralelEndCordsScreen = xPointScreen;
        else if (paralel == Axis::Y) paralelEndCordsScreen = yPointScreen;
        else if (paralel == Axis::Z) paralelEndCordsScreen = zPointScreen;

        auto cross2D = [](glm::vec2 a, glm::vec2 b) -> float {
            return std::abs(a.x * b.y - a.y * b.x);
        };

        const float xArea = cross2D(xPointScreen - centerPointScreen, paralelEndCordsScreen - centerPointScreen);
        const float yArea = cross2D(yPointScreen - centerPointScreen, paralelEndCordsScreen - centerPointScreen);
        const float zArea = cross2D(zPointScreen - centerPointScreen, paralelEndCordsScreen - centerPointScreen);

        if (xArea > yArea && xArea > zArea) return directions[0];
        else if (yArea > zArea) return directions[1];
        else return directions[2];
    };

    const float xAngle = glm::orientedAngle(glm::vec3(0.0f, 0.0f, -1.0f),
                                            findRotation(YZCenter, Axis::X),
                                            glm::vec3(1.0f, 0.0f, 0.0f));
    const float yAngle = glm::orientedAngle(glm::vec3(0.0f, 0.0f, -1.0f),
                                            findRotation(XZCenter, Axis::Y),
                                            glm::vec3(0.0f, -1.0f, 0.0f));
    const float zAngle = glm::orientedAngle(glm::vec3(1.0f, 0.0f, 0.0f),
                                            findRotation(XYCenter, Axis::Z),
                                            glm::vec3(0.0f, 0.0f, -1.0f));

    return Positioning{
        .xTransform = glm::translate(glm::mat4(1.0f), YZCenter)
                      * glm::rotate(glm::mat4(1.0f), xAngle, glm::vec3(1.0f, 0.0f, 0.0f)),
        .yTransform = glm::translate(glm::mat4(1.0f), XZCenter)
                      * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f))
                      * glm::rotate(glm::mat4(1.0f), yAngle, glm::vec3(-1.0f, 0.0f, 0.0f)),
        .zTransform = glm::translate(glm::mat4(1.0f), XYCenter)
                      * glm::rotate(glm::mat4(1.0f), zAngle, glm::vec3(0.0f, 0.0f, -1.0f))
                      * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f))
    };
}

std::array<Renderer::Vertex, 6> GeometryGenerator::generateLines(AABB edges, Positioning positioning, float offset) {
    std::array<Renderer::Vertex, 6> output{};

    const float xSize = edges.halfDimensions.x;
    output[0].pos = positioning.xTransform * glm::vec4(xSize, 0.0f, -offset, 1.0f);
    output[1].pos = positioning.xTransform * glm::vec4(-xSize, 0.0f, -offset, 1.0f);

    const float ySize = edges.halfDimensions.y;
    output[2].pos = positioning.yTransform * glm::vec4(ySize, 0.0f, -offset, 1.0f);
    output[3].pos = positioning.yTransform * glm::vec4(-ySize, 0.0f, -offset, 1.0f);

    const float zSize = edges.halfDimensions.z;
    output[4].pos = positioning.zTransform * glm::vec4(zSize, 0.0f, -offset, 1.0f);
    output[5].pos = positioning.zTransform * glm::vec4(-zSize, 0.0f, -offset, 1.0f);

    return output;
}

std::vector<Renderer::Vertex> GeometryGenerator::generateText(AABB edges, Positioning positioning, Renderer::UBOCamera camera, const Trex::Atlas& atlas, float offset) {
    const glm::mat4 viewProj = camera.proj * camera.view;

    std::vector<Renderer::Vertex> output;

    auto generateText = [&](const std::string_view& text, const glm::mat4& transform, bool isVertical) {
        Trex::TextShaper shaper(atlas);
        Trex::ShapedGlyphs glyphs = shaper.ShapeAscii(text);

        Trex::TextMeasurement measurments = Trex::TextShaper::Measure(glyphs);

        const float scale = (std::min(edges.halfDimensions.x, std::min(edges.halfDimensions.y, edges.halfDimensions.z)) * 0.2f) / (float)atlas.GetGlyphs().GetGlyphByCodepoint('0').height;

        const float width = measurments.width * scale;
        const float height = measurments.height * scale;

        const glm::vec4 directionStart = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        const glm::vec4 downEnd = glm::vec4(0.0f, 0.0f, -(float)atlas.GetGlyphs().GetGlyphByCodepoint('0').height * scale, 1.0f);
        const glm::vec4 rightEnd = glm::vec4((float)atlas.GetGlyphs().GetGlyphByCodepoint('0').width * scale, 0.0f, 0.0f, 1.0f);

        const auto directionStartScreen = toScreen(transform * directionStart, viewProj);
        const auto downEndScreen = toScreen(transform * downEnd, viewProj);
        const auto rightEndScreen = toScreen(transform * rightEnd, viewProj);

        bool xFlip;
        bool zFlip;

        if (isVertical) {
            xFlip = (rightEndScreen - directionStartScreen).y > 0;
            zFlip = (downEndScreen - directionStartScreen).x < 0;
        }
        else {
            xFlip = (rightEndScreen - directionStartScreen).x < 0;
            zFlip = (downEndScreen - directionStartScreen).y < 0;
        }

        const float xScale = xFlip ? -1.0f : 1.0f;
        const float zScale = zFlip ? -1.0f : 1.0f;

        glm::vec3 caretPos(-width / 2.0f * xScale, 0.0f, -((zFlip ? 0.0f : height) + offset));

        for (Trex::ShapedGlyph sym : glyphs) {
            glm::vec3 rightDirection(sym.info.width * scale * xScale, 0.0f, 0.0f);
            glm::vec3 downDirection(0.0f, 0.0f, -scale * sym.info.height * zScale);

            glm::vec2 topCorner((float)sym.info.x / (float)atlas.GetBitmap().Width(), (float)sym.info.y / (float)atlas.GetBitmap().Height());
            glm::vec2 bottomCorner(((float)sym.info.x + (float)sym.info.width) / (float)atlas.GetBitmap().Width(), ((float)sym.info.y + sym.info.height) / (float)atlas.GetBitmap().Height());


            std::array<Renderer::Vertex, 6> vertices{
                Renderer::Vertex{.pos = rightDirection + downDirection, .texCoord = glm::vec2{bottomCorner.x, bottomCorner.y}},  //right bottom
                Renderer::Vertex{.pos = rightDirection                , .texCoord = glm::vec2{bottomCorner.x, topCorner.y   }},  //right top
                Renderer::Vertex{.pos = downDirection                 , .texCoord = glm::vec2{topCorner.x   , bottomCorner.y}},  //left bottom
                Renderer::Vertex{.pos = rightDirection                , .texCoord = glm::vec2{bottomCorner.x, topCorner.y   }},  //right top
                Renderer::Vertex{.pos = {0, 0, 0}                     , .texCoord = glm::vec2{topCorner.x   , topCorner.y   }},  //left top
                Renderer::Vertex{.pos = downDirection                 , .texCoord = glm::vec2{topCorner.x   , bottomCorner.y}}   //left bottom
            };

            for (const auto vert : vertices) {
                output.push_back(Renderer::Vertex{
                    .pos = transform * glm::vec4(vert.pos + caretPos + glm::vec3(0.0f, sym.info.bearingX * scale * xScale, sym.info.bearingY * scale * zScale), 1.0f),
                    .texCoord = vert.texCoord
                });
            }

            caretPos += glm::vec3(sym.xAdvance * scale * xScale, 0.0f, sym.yAdvance * scale * zScale);
        }
    };

    generateText(std::format("{:.2f}", edges.halfDimensions.x) + " mm", positioning.xTransform, false);
    generateText(std::format("{:.2f}", edges.halfDimensions.y) + " mm", positioning.yTransform, false);
    generateText(std::format("{:.2f}", edges.halfDimensions.z) + " mm", positioning.zTransform, true);

    return output;
}
