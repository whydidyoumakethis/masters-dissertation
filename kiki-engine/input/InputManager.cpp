#include "InputManager.hpp"

namespace Kiki {
    void InputManager::initialise(GLFWwindow* targetWindow) {
        window = targetWindow;

        glfwSetKeyCallback(window, setKeyState);
        glfwSetMouseButtonCallback(window, setMouseButtonState);
        glfwSetCursorPosCallback(window, setMouseMovementState);

        for (int i = 0; i < GLFW_KEY_LAST; i++) {
            keyStates[i] = KeyState::RELEASED;
        }

        for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST; i++) {
            mouse.buttonStates[i] = KeyState::RELEASED;
        }
    }

    void InputManager::update() {        
        // transition keys on each frame
        for (int i = 0; i < GLFW_KEY_LAST; i++) {
            switch (keyStates[i]) {
                case KeyState::JUST_DOWN:
                    keyStates[i] = KeyState::DOWN;
                    break;
                case KeyState::JUST_RELEASED:
                    keyStates[i] = KeyState::RELEASED;
                    break;
                default:
                    break;
            }
        }

        // transition mouse buttons
        for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST; i++) {
            switch (mouse.buttonStates[i]) {
                case KeyState::JUST_DOWN:
                    mouse.buttonStates[i] = KeyState::DOWN;
                    break;
                case KeyState::JUST_RELEASED:
                    mouse.buttonStates[i] = KeyState::RELEASED;
                    break;
                default:
                    break;
            }
        }

        // reset mouse delta
        mouse.dx = 0.f;
        mouse.dy = 0.f;

        glfwPollEvents();
    }

    void InputManager::handleKey(GLFWwindow* window, int key, int scanCode, int action, int modifierFlags) {
        if (key < 0 || key > GLFW_KEY_LAST) {
            return;
        }

        if (action == GLFW_PRESS) {
            if (keyStates[key] == KeyState::RELEASED || keyStates[key] == KeyState::JUST_RELEASED) {
                keyStates[key] = KeyState::JUST_DOWN;
            }
        }
        else if (action == GLFW_RELEASE) {
            if (keyStates[key] == KeyState::DOWN || keyStates[key] == KeyState::JUST_DOWN) {
                keyStates[key] = KeyState::JUST_RELEASED;
            }
        }
    }

    void InputManager::handleMouseButton(GLFWwindow* window, int button, int action, int modifierFlags) {
        if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
            return;
        }

        if (action == GLFW_PRESS) {
            if (mouse.buttonStates[button] == KeyState::RELEASED || mouse.buttonStates[button] == KeyState::JUST_RELEASED) {
                mouse.buttonStates[button] = KeyState::JUST_DOWN;
            }
        }
        else if (action == GLFW_RELEASE) {
            if (mouse.buttonStates[button] == KeyState::DOWN || mouse.buttonStates[button] == KeyState::JUST_DOWN) {
                mouse.buttonStates[button] = KeyState::JUST_RELEASED;
            } 
        }
    }

    void InputManager::handleMouseMotion(GLFWwindow* window, float xPos, float yPos) {
        mouse.dx = xPos - mouse.x;
        mouse.dy = yPos - mouse.y;
        mouse.x = xPos;
        mouse.y = yPos;
    }

    bool InputManager::isKeyDown(int key) { return keyStates[key] == KeyState::DOWN || keyStates[key] == KeyState::JUST_DOWN; }
    bool InputManager::isKeyJustDown(int key) { return keyStates[key] == KeyState::JUST_DOWN; }
    bool InputManager::isKeyUp(int key) { return keyStates[key] == KeyState::RELEASED || keyStates[key] == KeyState::JUST_RELEASED; }
    bool InputManager::isKeyJustUp(int key) { return keyStates[key] == KeyState::JUST_RELEASED; }

    bool InputManager::isMouseButtonDown(int button) { return mouse.buttonStates[button] == KeyState::DOWN || mouse.buttonStates[button] == KeyState::JUST_DOWN; }
    bool InputManager::isMouseButtonJustDown(int button) { return mouse.buttonStates[button] == KeyState::JUST_DOWN; }
    bool InputManager::isMouseButtonUp(int button) { return mouse.buttonStates[button] == KeyState::RELEASED || mouse.buttonStates[button] == KeyState::JUST_RELEASED; }
    bool InputManager::isMouseButtonJustUp(int button) { return mouse.buttonStates[button] == KeyState::JUST_RELEASED; }

    void InputManager::getMousePosition(float &x, float &y) {
        x = mouse.x;
        y = mouse.y;
    }

    void InputManager::getMouseDeltaPosition(float &x, float &y) {
        x = mouse.dx;
        y = mouse.dy;
    }
}
