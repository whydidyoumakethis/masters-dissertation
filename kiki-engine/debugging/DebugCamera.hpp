#ifndef KIKI_DEBUGGING_DEBUGCAMERA
#define KIKI_DEBUGGING_DEBUGCAMERA

#include "../renderer/Camera.hpp"
#include "../input/InputManager.hpp"

namespace Kiki {
    class DebugCamera : public Camera {
        private:
        DebugCamera() = default;
        ~DebugCamera() = default;
        DebugCamera(const DebugCamera&) = delete;
        DebugCamera& operator=(const DebugCamera&) = delete;

        float lastX, lastY;

        Kiki::InputManager& inputManager = Kiki::InputManager::get();
        entt::registry& registry = World::Get().Registry();

        public:
        static DebugCamera& get();

        bool enabled = false;

        float speed = 10.f;
        float speedUp = 5.0f;
        float slowDown = 0.05f;
        float sensitivity = 0.75f;

        void update(float dt);
        void reset();
    };
}

#endif