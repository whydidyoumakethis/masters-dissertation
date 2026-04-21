#pragma once
#include <glm/glm.hpp>
struct CameraComponent {
    float fov = 90.0f;
    float nearPlane = 0.1f;
    float farPlane = 2000.0f;
    bool  isMain = false;   // mark the main camera

    // reserve
    //glm::mat4 viewMatrix = glm::mat4(1.0f);
    //glm::mat4 projectionMatrix = glm::mat4(1.0f);
};