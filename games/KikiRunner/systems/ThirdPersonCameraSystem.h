#pragma once
#include <kiki.h>
#include "components/CharacterComponent.h"
#include "components/ThirdPersonCameraComponent.hpp"
#include "events/ResetThirdPersonCameraEvent.hpp"

class ThirdPersonCamera : public Camera {
public:
	ThirdPersonCamera() : Camera() {
		auto& reg = World::Get().Registry();
		reg.emplace<ThirdPersonCameraComponent>(camera);
	}

};
class ThirdPersonCameraSystem : public System {
public:
	Phase GetPhase()    const override { return Phase::Update; }
	void OnUpdate(float dt) override {
		if (inputManager.isKeyJustDown(GLFW_KEY_SPACE) && !inputManager.isCursorDisabledFunc()) {
			inputManager.disableCursor();
			//spdlog::info("Right mouse button down, cursor disabled");
		}
		auto objects = World::Get().Query<TransformComponent, ThirdPersonCameraComponent, CameraComponent>();
		for (auto [entity, transform, tpscam,cam] : objects.each()) {
			HandleCameraRotation( tpscam,transform);
			HandleCameraZoom(tpscam);
			HandleCameraPosition(transform, cam, tpscam, dt);
		
		}
	}
	void OnStart() override {
		Reset();
		MessageCenter::Subscribe<ResetThirdPersonCameraEvent, &ThirdPersonCameraSystem::OnResetEvent>(this);
	}

	void OnResetEvent(const ResetThirdPersonCameraEvent& e) {
		Reset();
	}
	
private:
	InputManager& inputManager = Kiki::InputManager::get();
	ThirdPersonCamera camera;
	bool isDisablingCursor = false;
	void HandleCameraRotation(ThirdPersonCameraComponent& cam, TransformComponent& trans)
	{
		float mousedX = 0.0f, mousedY = 0.0f;
		if (inputManager.isCursorDisabledFunc()) {
			inputManager.getMouseDeltaPosition(mousedX, mousedY);
		}
		if (mousedX < 0.001f && mousedX > -0.001f && 
			mousedY < 0.001f && mousedY > -0.001f) return;

		cam.yaw -= mousedX * cam.rotateSensitivity;
		cam.pitch += mousedY * cam.rotateSensitivity;
		cam.pitch = glm::clamp(cam.pitch, cam.minPitch, cam.maxPitch);
		//glm::quat rotation = glm::quat(glm::radians(glm::vec3(cam.pitch, cam.yaw, 0.0f)));
		//trans.rotation = rotation;
		//trans.dirty = true;
	}

	void Reset() {
		auto objects = World::Get().Query<CharacterComponent, PhysicalAttributesComponent>();

		if (auto tpcc = World::Get().GetComponent<ThirdPersonCameraComponent>(camera.camera)) {
			for (auto [entity, chara, phys] : objects.each()) {
				tpcc->followTarget = entity;
				spdlog::info("Camera follow target locked to entity ID: {}", (uint32_t)entity);
				break;
			}
		}
		World::Get().GetComponent<CameraComponent>(camera.camera)->isMain = true;
	}

	void HandleCameraZoom(ThirdPersonCameraComponent& cam) {
		float scroll = inputManager.getMouseScrollDelta();
		if (scroll == 0.0f) return;
		cam.distance -= scroll * cam.zoomSensitivity;
		cam.distance = glm::clamp(cam.distance, cam.minDistance, cam.maxDistance);
	}

	void HandleCameraPosition(TransformComponent& camTransform,
		CameraComponent& camera,
		ThirdPersonCameraComponent& tpsCam,
		float dt)
	{
		if (tpsCam.followTarget == NullEntity) return;

		auto* targetTransform = World::Get().GetComponent<TransformComponent>(tpsCam.followTarget);
		if (!targetTransform) return;

		// follow point = character position + height offset
		glm::vec3 pivotPos = targetTransform->position
			+ glm::vec3(0, tpsCam.height, 0);

		float yawRad = glm::radians(tpsCam.yaw);
		float pitchRad = glm::radians(tpsCam.pitch);

		glm::vec3 offset = {
			tpsCam.distance * cos(pitchRad) * sin(yawRad),
			tpsCam.distance * sin(pitchRad),
			tpsCam.distance * cos(pitchRad) * cos(yawRad)
		};

		glm::vec3 desiredPos = pivotPos + offset;

		// smooth Interpolation (SmoothDamp effect)
		tpsCam.currentPos = glm::mix(
			tpsCam.currentPos,
			desiredPos,
			tpsCam.smoothSpeed * dt
		);

		camTransform.position = tpsCam.currentPos;
		camTransform.dirty = true;
		glm::vec3 forward = glm::normalize(pivotPos - camTransform.position);
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

		camTransform.rotation = glm::quatLookAt(forward, up);
		//camTransform.forward = glm::normalize(pivotPos - camTransform.position);
	}
};