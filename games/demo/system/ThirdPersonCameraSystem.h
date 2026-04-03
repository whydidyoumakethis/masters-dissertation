#pragma once
#include <kiki.h>
#include "../component/CharacterComponent.h"
#include "../component/ThirdPersonCameraComponent.hpp"
class ThirdPersonCamera : public Camera {
	ThirdPersonCamera() : Camera() {
		auto& reg = World::Get().Registry();
		reg.emplace<ThirdPersonCameraComponent>(camera);
	}

};
class ThirdPersonCameraSystem : public System {
public:
	Phase GetPhase()    const override { return Phase::Update; }
	void OnUpdate(float dt) override {
		auto objects = World::Get().Query<TransformComponent, ThirdPersonCameraComponent>();
		for (auto [entity, transform, cam] : objects.each()) {
		
		
		}
	}
	void OnStart() override {
		auto objects = World::Get().Query< CharacterComponent >();
		if (auto tpcc = World::Get().Registry().try_get<ThirdPersonCameraComponent>(camera.camera)) {
			// if consider mutiple characters, we can add a condition to determine which character to follow, for now we just follow the first one we find
			for (auto [entity, i] : objects.each()) {
				tpcc->followTarget = entity;
			}
		}
	}
	void HandleCameraRotation(ThirdPersonCameraComponent& cam, TransformComponent& trans)
	{
		float mousedX, mousedY;
		inputManager.getMouseDeltaPosition(mousedX, mousedY);
		if (mousedX < 0.001f && mousedY < 0.001f) return;

		cam.yaw += mousedX * cam.rotateSensitivity;
		cam.pitch += mousedY * cam.rotateSensitivity;
		cam.pitch = glm::clamp(cam.pitch, cam.minPitch, cam.maxPitch);
		glm::quat rotation = glm::quat(glm::radians(glm::vec3(cam.pitch, cam.yaw, 0.0f)));
		trans.rotation = rotation;
	}
private:
	InputManager& inputManager = Kiki::InputManager::get();
	ThirdPersonCamera camera;
};