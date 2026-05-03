#pragma once
#include <kiki.h>
#include "events/TimerTriggerEvent.h"
#include "events/ResetLevelEvent.hpp"

class GoalTriggerSystem : public System {
public:
	Phase GetPhase() const override { return Phase::PostUpdate; }
	void OnUpdate(float dt) override {
		if (goalReached) {
			goalReached = false; // reset the flag for next time
			auto* playerTransform = World::Get().GetComponent<TransformComponent>(playerEntity);
			auto* cc = World::Get().GetComponent<CharacterComponent>(playerEntity);
			auto spawnPos = cc->spawnPosition;
			if (playerTransform) {
				playerTransform->position = spawnPos;
				spdlog::info("Resetting player position to spawn point: x={}, y={}, z={}", spawnPos.x, spawnPos.y, spawnPos.z);
				playerTransform->dirty = true;
			}
		}
	}
	void OnStart() override {
		
		//MessageCenter::Subscribe<CollisionEvent, OnTriggerEnter>();
		MessageCenter::Subscribe<CollisionEvent, &GoalTriggerSystem::OnTriggerEnter>(this);
		MessageCenter::Subscribe<ResetLevelEvent, &GoalTriggerSystem::OnLevelReset>(this);
		Reset();
	}

	void Reset() {
		auto objects = World::Get().Query<MiscComponent>();
		for (auto [e, misc] : objects.each()) {
			if (misc.miscTag == MmiscTags::GOAL) {
				goalEntity = e;
				//spdlog::info("Goal entity found: {}", (uint32_t)e);
			}
			if (misc.miscTag == MmiscTags::PLAYER) {
				playerEntity = e;
				//spdlog::info("Player entity found: {}", (uint32_t)e);
			}
		}
	}

	void OnLevelReset(const ResetLevelEvent& e) {
		Reset();
	}

	void OnTriggerEnter(const CollisionEvent& e) {
		auto& reg = World::Get().Registry();
		//spdlog::info("Collision detected between entity {} and entity {}", (uint32_t)e.entity1, (uint32_t)e.entity2);
		
		if ((e.entity1 == playerEntity && e.entity2 == goalEntity) || (e.entity1 == goalEntity && e.entity2 == playerEntity)) {
			//spdlog::info("Player reached the goal! You win!");
			MessageCenter::Publish(TimerTriggerEvent{ Timer::get().Elapsed() });
			Timer::get().Reset();
			goalReached = true;
		}
	}
private:
	bool goalReached = false;
	Entity playerEntity = NullEntity;
	Entity goalEntity = NullEntity;
};