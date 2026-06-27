#include "types.h"

#include <glm/gtc/matrix_transform.hpp>



Renderer::UBOCamera::UBOCamera(float yaw, float pitch, float distance, float height, float fov, float aspectRatio) {
    glm::vec3 lookVector = glm::rotate(glm::mat4(1.0f), glm::radians(yaw), glm::vec3(0.0f, 0.0f, 1.0f))
        * glm::rotate(glm::mat4(1.0f), glm::radians(pitch), glm::vec3(0.0f, 1.0f, 0.0f))
        * glm::vec4(distance, 0.0f, 0.0f, 0.0f);

    glm::vec3 cameraPosition = -lookVector;
    cameraPosition.z += height;

    view = glm::lookAt(cameraPosition, cameraPosition + lookVector, glm::vec3(0.0f, 0.0f, 1.0f));
    proj = glm::perspective(glm::radians(fov), aspectRatio, 0.01f, 30.0f);
    proj[1][1] *= -1;
}