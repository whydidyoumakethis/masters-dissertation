#ifndef KIKI_INTERFACE_INTERFACESYSTEM
#define KIKI_INTERFACE_INTERFACESYSTEM

#include "Components/InterfaceComponent.hpp"
#include "Components/BackgroundComponent.hpp"
#include "Components/TextComponent.hpp"
#include "Components/ButtonComponent.hpp"
#include "events/ButtonClickEvent.hpp"
#include "events/ButtonHoverEvent.hpp"
#include "Components/InterfaceAnimationComponent.hpp"
#include "Components/AspectRatioComponent.hpp"

#include "ECS/World.h"
#include "ECS/System.h"
#include "FontManager.hpp"
#include "TextureManager.hpp"
#include "RenderManager.hpp"
#include "MessageCenter.h"
#include "InputManager.hpp"

namespace Kiki {
    class InterfaceSystem : public System {
        private:
        FontManager& fontManager = FontManager::get();
        TextureManager& textureManager = TextureManager::get();
        RenderManager& renderManager = RenderManager::get();
        MessageCenter& messageCenter = MessageCenter::Get();
        InputManager& inputManager = InputManager::get();

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