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
        
        inputManager.update();

        if (inputManager.isGamepadConnected()) {
 
			//A Button
            if (inputManager.isGamepadButtonJustDown(GLFW_GAMEPAD_BUTTON_A)) {
                spdlog::warn("Gamepad: A Button Jump!");
            }

            //RT
            float rt = inputManager.getGamepadAxis(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER);
            if (rt > 0.1f) {
                spdlog::info("Gas Pedal: {:.2f}", rt);
            }

			//left stick
            float lx = inputManager.getGamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_X);
            float ly = inputManager.getGamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_Y);
            if (std::abs(lx) > 0.1f || std::abs(ly) > 0.1f) {
                spdlog::info("Moving Stick: X={:.2f}, Y={:.2f}", lx, ly);
            }
        }
        
		// handle window closing
        //if (inputManager.isKeyJustDown(GLFW_KEY_ESCAPE) ) {
        //    if (!inputManager.isCursorDisabledFunc()) {
        //        glfwSetWindowShouldClose(RenderManager::get().getWindow(), true);
        //    }
        //    else inputManager.enableCursor();
        //    //spdlog::info("Escape key down, cursor enabled");
        //}

        // example usage
        //if (inputManager.isKeyJustDown(GLFW_KEY_E)) {
        //    spdlog::info("E key was just pressed");
        //}
        //else if (inputManager.isKeyDown(GLFW_KEY_E)) {
        //    spdlog::info("E key is pressed");
        //}
        //else if (inputManager.isKeyJustUp(GLFW_KEY_E)) {
        //    spdlog::info("E key was just released");
        //}

        //float mouseX = 0.f;
        //float mouseY = 0.f;
        //inputManager.getMousePosition(mouseX, mouseY);
        //// spdlog::info("Mouse position: ({0}, {1})", mouseX, mouseY);

        //if (inputManager.isMouseButtonJustDown(GLFW_MOUSE_BUTTON_RIGHT)) {
        //    spdlog::info("Right click was just pressed");
        //}
        //else if (inputManager.isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
        //    spdlog::info("Right click is pressed");
        //}
        //else if (inputManager.isMouseButtonJustUp(GLFW_MOUSE_BUTTON_RIGHT)) {
        //    spdlog::info("Right click was just released");
        //}
    }
}