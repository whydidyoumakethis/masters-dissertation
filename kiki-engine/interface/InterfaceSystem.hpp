#ifndef KIKI_INTERFACE_INTERFACESYSTEM
#define KIKI_INTERFACE_INTERFACESYSTEM

#include "ECS/World.h"
#include "ECS/System.h"
#include "FontManager.hpp"
#include "TextureManager.hpp"

namespace Kiki {
    struct ScaleVec2D {
        float scaleX;
        float x;
        float scaleY;
        float y;
        bool dirty = true;

        // Absolute values should only be changed by the InterfaceSystem
        float absoluteX;
        float absoluteY;
    };

    class InterfaceSystem : public System {
        private:
        FontManager& fontManager = FontManager::get();
        TextureManager& textureManager = TextureManager::get();

        World& world = World::Get();

        public:
        Phase GetPhase() const override { return Phase::Input; }

        void OnStart() override;

        void OnUpdate(float dt) override;

        void OnStop() override;
	};
}

#endif