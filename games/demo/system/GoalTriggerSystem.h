#pragma once
#include <kiki.h>

class GoalTriggerSystem : public System {
public:
	Phase GetPhase() const override { return Phase::Physics; }
	void OnUpdate(float dt) override {
		
	}
	void OnStart() override {
		
		//MessageCenter::Subscribe<CollisionEvent, OnTriggerEnter>();
		MessageCenter::Subscribe<CollisionEvent, &GoalTriggerSystem::OnTriggerEnter>(this);
		auto objects = World::Get().Query<MiscComponent>();
		for(auto[e,misc] : objects.each()){
			if(misc.miscTag == MmiscTags::GOAL){
				goalEntity = e;
				//spdlog::info("Goal entity found: {}", (uint32_t)e);
			}
			if (misc.miscTag == MmiscTags::PLAYER) {
				playerEntity = e;
				//spdlog::info("Player entity found: {}", (uint32_t)e);
			}
		}
	}
	void OnTriggerEnter(const CollisionEvent& e) {
		auto& reg = World::Get().Registry();
		//spdlog::info("Collision detected between entity {} and entity {}", (uint32_t)e.entity1, (uint32_t)e.entity2);
		
		if ((e.entity1 == playerEntity && e.entity2 == goalEntity) || (e.entity1 == goalEntity && e.entity2 == playerEntity)) {
			spdlog::info("Player reached the goal! You win!");
			auto* playerTransform = reg.try_get<TransformComponent>(playerEntity);
			auto spawnPos = reg.try_get<CharacterComponent>(playerEntity)->spawnPosition;
			if (playerTransform) {
				playerTransform->position = spawnPos;
				playerTransform->dirty = true;
			}
		}
	}
private:
	Entity playerEntity = NullEntity;
	Entity goalEntity = NullEntity;
};