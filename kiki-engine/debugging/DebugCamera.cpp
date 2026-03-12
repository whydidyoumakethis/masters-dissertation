#include "DebugCamera.hpp"

#include "../input/InputManager.hpp"
#include "../renderer/SceneManager.hpp"

namespace Kiki {
    void DebugCamera::update(float dt) {
        TransformComponent& transform = registry.get<TransformComponent>(camera);

        float const move = dt * speed * (inputManager.isKeyDown(GLFW_KEY_LEFT_CONTROL) ? speedUp:1.0f) * (inputManager.isKeyDown(GLFW_KEY_LEFT_SHIFT) ? slowDown:1.0f);

        if (inputManager.isKeyDown(GLFW_KEY_W))
            transform.position += glm::vec3(0.0f, 0.0f, move);
        if (inputManager.isKeyDown(GLFW_KEY_S))
            transform.position += glm::vec3(0.0f, 0.0f, -move);
        if (inputManager.isKeyDown(GLFW_KEY_A))
            transform.position += glm::vec3(move, 0.0f, 0.0f);
        if (inputManager.isKeyDown(GLFW_KEY_D))
            transform.position += glm::vec3(-move, 0.0f, 0.0f);
        if (inputManager.isKeyDown(GLFW_KEY_E))
            transform.position += glm::vec3(0.0f, -move, 0.0f);
        if (inputManager.isKeyDown(GLFW_KEY_Q))
            transform.position += glm::vec3(0.0f, move, 0.0f);

        if (inputManager.isKeyDown(GLFW_KEY_F))
            SceneManager::get().clearLevel();
    }
}