#pragma once
#include <kiki.h>
#include "../component/CharacterComponent.h"
#include "../component/ThirdPersonCameraComponent.hpp"
class CharacterSystem : public System {
public:
    Phase GetPhase() const override { return Phase::Update; }

    void OnUpdate(float dt) override {
        auto objects = World::Get().Query<TransformComponent, CharacterComponent>();
		for (auto [entity, transform, character] : objects.each()) {
            float cameraYaw = GetCameraYaw(entity);

        }
        auto& inputManager = Kiki::InputManager::get();
    }
    void OnStart() override {
        
    }
    void OnStop() override {
       
    }
private:
    float GetCameraYaw(Entity targetEntity) {
        float yaw = 0.0f;
        auto camView = World::Get().Query<ThirdPersonCameraComponent>();
        for (auto [e, cam] : camView.each()) {
            if (cam.followTarget == targetEntity) {
                yaw = cam.yaw;
                break;
            }
        }
        return yaw;
    }
};