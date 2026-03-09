#include "InputSystem.hpp"

#include <spdlog/spdlog.h>

namespace Kiki {
    void InputSystem::OnStart() {
        spdlog::info("Starting input system");

        // input manager needs the window from the render manager
        Kiki::RenderManager& renderManager = Kiki::RenderManager::get();

        inputManager.initialise(renderManager.getWindow());
    }

    void InputSystem::OnUpdate(float dt) {
        // example usage
        if (inputManager.isKeyJustDown(GLFW_KEY_E)) {
            spdlog::info("E key was just pressed");
        }
        else if (inputManager.isKeyDown(GLFW_KEY_E)) {
            spdlog::info("E key is pressed");
        }
        else if (inputManager.isKeyJustUp(GLFW_KEY_E)) {
            spdlog::info("E key was just released");
        }

        float mouseX = 0.f;
        float mouseY = 0.f;
        inputManager.getMousePosition(mouseX, mouseY);
        // spdlog::info("Mouse position: ({0}, {1})", mouseX, mouseY);

        if (inputManager.isMouseButtonJustDown(GLFW_MOUSE_BUTTON_RIGHT)) {
            spdlog::info("Right click was just pressed");
        }
        else if (inputManager.isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
            spdlog::info("Right click is pressed");
        }
        else if (inputManager.isMouseButtonJustUp(GLFW_MOUSE_BUTTON_RIGHT)) {
            spdlog::info("Right click was just released");
        }

        inputManager.update();
    }
}