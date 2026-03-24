#ifndef KIKI_DEBUGGING_DEBUGCAMERA
#define KIKI_DEBUGGING_DEBUGCAMERA

#include "../renderer/Camera.hpp"
#include "../input/InputManager.hpp"

namespace Kiki {
    class DebugCamera : public Camera {
        private:
        float speed = 50.f;
        float speedUp = 5.0f;
        float slowDown = 0.05f;
        float sensitivity = 0.75f;
        float lastX, lastY;

        Kiki::InputManager& inputManager = Kiki::InputManager::get();
        entt::registry& registry = World::Get().Registry();

        public:
        void update(float dt);
    };
}

#endif