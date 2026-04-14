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

        float lastX, lastY, yaw, pitch;

        Kiki::InputManager& inputManager = Kiki::InputManager::get();
        World& world = World::Get();
        entt::registry& registry = world.Registry();

        public:
        static DebugCamera& get();

        bool enabled = false;

        float speed = 10.f;
        float speedUp = 5.0f;
        float slowDown = 0.05f;
        float sensitivity = 0.25f;

        void update(float dt);
        void reset();
        void enter();
    };
}

#endif