#ifndef KIKI_INPUT_INPUTMANAGER
#define KIKI_INPUT_INPUTMANAGER

#if !defined(GLFW_INCLUDE_NONE)
#	define GLFW_INCLUDE_NONE 1
#endif
#include <GLFW/glfw3.h>

#include <iostream>

namespace Kiki {
    class InputManager {
        private:
        enum KeyState {
            JUST_DOWN,
            DOWN,
            JUST_RELEASED,
            RELEASED
        };

        struct MouseState {
            KeyState buttonStates[GLFW_MOUSE_BUTTON_LAST];
            float x = 0.f;
            float y = 0.f;
            float dx = 0.f;
            float dy = 0.f;
            float scroll = 0.f;
        };

        struct GamepadState {
            bool connected = false;
            KeyState buttonStates[GLFW_GAMEPAD_BUTTON_LAST + 1];
            float axes[GLFW_GAMEPAD_AXIS_LAST + 1];
        };

        GamepadState gamepad;

        float deadzone = 0.15f;
		bool isCursorDisabled = false;
        InputManager() = default;
        ~InputManager() = default;
        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;

        GLFWwindow* window;

        KeyState keyStates[GLFW_KEY_LAST]{};
        MouseState mouse;

        // glfw doesn't want 'self', it just wants the other arguments
        // so use a static function and forward it
        static void setKeyState(GLFWwindow* window, int key, int scanCode, int action, int modifierFlags) {
            InputManager::get().handleKey(window, key, scanCode, action, modifierFlags);
        }

        static void setMouseButtonState(GLFWwindow* window, int button, int action, int modifierFlags) {
            InputManager::get().handleMouseButton(window, button, action, modifierFlags);
        }

        static void setMouseMovementState(GLFWwindow* window, double xPos, double yPos) {
            InputManager::get().handleMouseMotion(window, (float)xPos, (float)yPos);
        }

        static void setMouseScrollState(GLFWwindow* window, double xOffset, double yOffset) {
            InputManager::get().handleMouseScroll(window, (float)xOffset, (float)yOffset);
        }

        void handleKey(GLFWwindow* window, int key, int scanCode, int action, int modifierFlags);
        void handleMouseButton(GLFWwindow* window, int button, int action, int modifierFlags);
        void handleMouseMotion(GLFWwindow* window, float xPos, float yPos);
        void handleMouseScroll(GLFWwindow* window, float xOffset, float yOffset);

        public:
        static InputManager& get() {
            static InputManager instance;
            return instance;
        }

        void initialise(GLFWwindow* targetWindow);
        void update();
    
        bool isKeyDown(int key);
        bool isKeyJustDown(int key);
        bool isKeyUp(int key);
        bool isKeyJustUp(int key);

        bool isMouseButtonDown(int button);
        bool isMouseButtonJustDown(int button);
        bool isMouseButtonUp(int button);
        bool isMouseButtonJustUp(int button);

        void getMousePosition(float &x, float &y);
        void getMouseDeltaPosition(float &x, float &y);
        float getMouseScrollDelta();

		//FOR GAMEPAD
        bool isGamepadConnected() const { return gamepad.connected; }

        bool isGamepadButtonDown(int button);
        bool isGamepadButtonJustDown(int button);
        bool isGamepadButtonUp(int button);
        bool isGamepadButtonJustUp(int button);

		float getGamepadAxis(int axis); //FROM -1.0 to 1.0
        void setGamepadDeadzone(float value) { deadzone = value; }

        // Enable/disable cursor for the window
        void disableCursor();
        void enableCursor();
		bool isCursorDisabledFunc() const { return isCursorDisabled; }
    };
}

#endif