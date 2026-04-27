#ifndef KIKI_INTERFACE_INTERFACESYSTEM
#define KIKI_INTERFACE_INTERFACESYSTEM

#include "ECS/World.h"
#include "ECS/System.h"
#include "FontManager.hpp"
#include "TextureManager.hpp"
#include "RenderManager.hpp"

namespace Kiki {
    struct ScaleVec2D {
        float scaleX;
        float x;
        float scaleY;
        float y;

        // Absolute values should only be changed by the InterfaceSystem
        float absoluteX;
        float absoluteY;
    };

    class InterfaceSystem : public System {
        private:
        FontManager& fontManager = FontManager::get();
        TextureManager& textureManager = TextureManager::get();
        RenderManager& renderManager = RenderManager::get();

        World& world = World::Get();
        entt::registry& registry = world.Registry();

        public:
        Phase GetPhase() const override { return Phase::PreUpdate; }

        void OnStart() override;

        void OnUpdate(float dt) override;

        void OnStop() override;
	};
}

#endif