#include "DebugCamera.hpp"

#include "../input/InputManager.hpp"
#include "../renderer/SceneManager.hpp"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Kiki {
    DebugCamera& DebugCamera::get() {
        static DebugCamera instance;
        return instance;
    }

    void DebugCamera::update(float dt) {
        if (registry.valid(camera)) {
            TransformComponent& transform = registry.get<TransformComponent>(camera);

            if (inputManager.isMouseButtonJustDown(GLFW_MOUSE_BUTTON_RIGHT))
                inputManager.disableCursor();
            if (inputManager.isMouseButtonJustUp(GLFW_MOUSE_BUTTON_RIGHT))
                inputManager.enableCursor();

            float x, y;

            inputManager.getMousePosition(x, y);

            if (inputManager.isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT) && !inputManager.isMouseButtonJustDown(GLFW_MOUSE_BUTTON_RIGHT)) {
                float const dx = x - lastX;
                float const dy = y - lastY;

                yaw += sensitivity * -dx;
                pitch = std::clamp(pitch + (sensitivity * -dy), -89.0f, 89.0f);
            }

            transform.rotation = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), 0.0f));

            lastX = x;
            lastY = y;

            float const move = dt * speed * (inputManager.isKeyDown(GLFW_KEY_LEFT_CONTROL) ? speedUp:1.0f) * (inputManager.isKeyDown(GLFW_KEY_LEFT_SHIFT) ? slowDown:1.0f);

            if (inputManager.isKeyDown(GLFW_KEY_W))
                transform.position += transform.rotation * glm::vec3(0.0f, 0.0f, -move);
            if (inputManager.isKeyDown(GLFW_KEY_S))
                transform.position += transform.rotation * glm::vec3(0.0f, 0.0f, move);
            if (inputManager.isKeyDown(GLFW_KEY_A))
                transform.position += transform.rotation * glm::vec3(-move, 0.0f, 0.0f);
            if (inputManager.isKeyDown(GLFW_KEY_D))
                transform.position += transform.rotation * glm::vec3(move, 0.0f, 0.0f);
            if (inputManager.isKeyDown(GLFW_KEY_E))
                transform.position += transform.rotation * glm::vec3(0.0f, move, 0.0f);
            if (inputManager.isKeyDown(GLFW_KEY_Q))
                transform.position += transform.rotation * glm::vec3(0.0f, -move, 0.0f);

            transform.dirty = true;
        } else {
            reset();
        }
    }

    void DebugCamera::reset() {
        if (registry.valid(camera))
            world.DestroyEntity(camera);

        camera = world.CreateEntity();

        registry.emplace<TransformComponent>(camera);
        registry.emplace<TagComponent>(camera, entt::hashed_string("DebugCamera"), "DebugCamera");
        registry.emplace<CameraComponent>(camera);
        registry.get<TransformComponent>(camera).position = glm::vec3( 0.f, 0.3f, 1.f);
        registry.get<TransformComponent>(camera).dirty = true;
    }

    void DebugCamera::enter() {
        auto cameras = world.Query<CameraComponent>();
        auto& cameraComp = registry.get_or_emplace<CameraComponent>(camera);
        auto& transformComp = registry.get_or_emplace<TransformComponent>(camera);

        for (auto cam : cameras) {
            auto comp = registry.get<CameraComponent>(cam);

            if (comp.isMain) {
                cameraComp.fov = comp.fov;
                cameraComp.nearPlane = comp.nearPlane;
                cameraComp.farPlane = comp.farPlane;

                if (registry.all_of<TransformComponent>(cam)) {
                    auto tempTransformComp = world.GetComponent<TransformComponent>(cam);
                    transformComp.position = tempTransformComp->position;
                    transformComp.rotation = tempTransformComp->rotation;
                    transformComp.dirty = true;
                }

                break;
            }
        }

        glm::vec3 forward = glm::normalize(transformComp.rotation * glm::vec3(0, 0, -1.0f));

        pitch = glm::degrees(asin(forward.y));
        yaw = -glm::degrees(atan2(forward.x, -forward.z));

        enabled = true;
    }
}