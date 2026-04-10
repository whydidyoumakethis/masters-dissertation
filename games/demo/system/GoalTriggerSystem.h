#pragma once
#include <kiki.h>

class GoalTriggerSystem : public System {
public:
	Phase GetPhase() const override { return Phase::Physics; }
	void OnUpdate(float dt) override {
		
	}
	void OnStart() override {
		MessageCenter::Subscribe<CollisionEvent, &OnTriggerEnter>();
	}
	static void OnTriggerEnter(const CollisionEvent& e) {
		auto& reg = World::Get().Registry();
		spdlog::info("Collision detected between entity {} and entity {}", (uint32_t)e.entity1, (uint32_t)e.entity2);
	}
};