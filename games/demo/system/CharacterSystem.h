#pragma once
#include <kiki.h>
#include "../component/CharacterComponent.h"
class CharacterSystem : public System {
public:
    Phase GetPhase() const override { return Phase::Update; }

    void OnUpdate(float dt) override {
        auto objects = World::Get().Query<TransformComponent, CharacterComponent>();

    }
    void OnStart() override {
        
    }
    void OnStop() override {
       
    }
private:
    
};