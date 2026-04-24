#ifndef KIKI_DEBUGGING_ENTITYVIEWER
#define KIKI_DEBUGGING_ENTITYVIEWER

#include "ECS/World.h"

namespace debug {
    class EntityViewer {
        public:
        static EntityViewer& get();
        void draw(bool* visible);

        private:
        EntityViewer() = default;
        ~EntityViewer() = default;
        EntityViewer(const EntityViewer&) = delete;
        EntityViewer& operator=(const EntityViewer&) = delete;

        World& world = World::Get();
        entt::registry& registry = world.Registry();
        entt::entity selectedEntity = entt::null;

        void deleteSelected();
        void destroyAllEntities();
        void destroyMeshEntities();
    };
}

#endif