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

        InputManager() = default;
        ~InputManager() = default;
        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;


        public:
        static InputManager& get() {
            static InputManager instance;
            return instance;
        }

        void initialise() {
            for (int i = 0; i < GLFW_KEY_LAST; i++) {
                keyStates[i] = KeyState::RELEASED;
            }
        }
    
        bool isKeyDown(int key);
        bool isKeyJustDown(int key);
        bool isKeyUp(int key);
        bool isKeyJustUp(int key);
        KeyState keyStates[GLFW_KEY_LAST]{};
    };
}

#endif