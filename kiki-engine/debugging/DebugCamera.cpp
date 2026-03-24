#include "DebugCamera.hpp"

#include "../input/InputManager.hpp"
#include "../renderer/SceneManager.hpp"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Kiki {
    void DebugCamera::update(float dt) {
        TransformComponent& transform = registry.get<TransformComponent>(camera);
        glm::mat4 translation = transform.worldMatrix;

        if (inputManager.isMouseButtonJustDown(GLFW_MOUSE_BUTTON_RIGHT))
            inputManager.disableCursor();
        if (inputManager.isMouseButtonJustUp(GLFW_MOUSE_BUTTON_RIGHT))
            inputManager.enableCursor();

        float x, y;

        inputManager.getMousePosition(x, y);

        if (inputManager.isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT) && !inputManager.isMouseButtonJustDown(GLFW_MOUSE_BUTTON_RIGHT)) {
            float const dx = dt * sensitivity * (x - lastX);
            float const dy = dt * sensitivity * (y - lastY);

            transform.worldMatrix *= glm::rotate(-dx, glm::vec3(0.0f, 1.0f, 0.0f));
            transform.worldMatrix *= glm::rotate(-dy, glm::vec3(1.0f, 0.0f, 0.0f));
        }

        lastX = x;
        lastY = y;

        float const move = dt * speed * (inputManager.isKeyDown(GLFW_KEY_LEFT_CONTROL) ? speedUp:1.0f) * (inputManager.isKeyDown(GLFW_KEY_LEFT_SHIFT) ? slowDown:1.0f);

        if (inputManager.isKeyDown(GLFW_KEY_W))
            transform.worldMatrix *= glm::translate(glm::vec3(0.0f, 0.0f, -move));
        if (inputManager.isKeyDown(GLFW_KEY_S))
            transform.worldMatrix *= glm::translate(glm::vec3(0.0f, 0.0f, move));
        if (inputManager.isKeyDown(GLFW_KEY_A))
            transform.worldMatrix *= glm::translate(glm::vec3(-move, 0.0f, 0.0f));
        if (inputManager.isKeyDown(GLFW_KEY_D))
            transform.worldMatrix *= glm::translate(glm::vec3(move, 0.0f, 0.0f));
        if (inputManager.isKeyDown(GLFW_KEY_E))
            transform.worldMatrix *= glm::translate(glm::vec3(0.0f, move, 0.0f));
        if (inputManager.isKeyDown(GLFW_KEY_Q))
            transform.worldMatrix *= glm::translate(glm::vec3(0.0f, -move, 0.0f));

        if (inputManager.isKeyDown(GLFW_KEY_F))
            SceneManager::get().clearLevel();
    }
}