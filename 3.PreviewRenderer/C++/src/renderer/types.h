#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>



namespace Renderer {

struct Vertex {
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 normal;
    alignas(16) glm::vec2 texCoord;
};

struct UBOCamera {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;

    UBOCamera(float yaw, float pitch, float distance, float height,
        float fov, float aspectRatio);
};

struct VertexPushConstant {
    alignas(16) vk::Bool32 enableShading;
};

}