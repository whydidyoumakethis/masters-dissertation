#ifndef KIKI_RENDERER_CAMERA
#define KIKI_RENDERER_CAMERA

#include "../ECS/World.h"
#include "../ECS/GameObject.h"

#include <entt/entt.hpp>

namespace Kiki {
    class Camera {
        public:
        entt::entity camera;

        Camera() {
            camera = World::Get().CreateEntity();

            auto& registry = World::Get().Registry();
            registry.emplace<TransformComponent>(camera);
            registry.emplace<TagComponent>(camera, entt::hashed_string("camera"), "camera");
            registry.get<TransformComponent>(camera).position = glm::vec3( 0.f, 0.3f, 1.f);
            registry.get<TransformComponent>(camera).dirty = true;
        }

        ~Camera() {
            World::Get().DestroyEntity(camera);
        }
    };
}

#endif