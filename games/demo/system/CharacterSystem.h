#pragma once
#include <kiki.h>
#include "../component/CharacterComponent.h"
#include "../component/ThirdPersonCameraComponent.hpp"
class CharacterSystem : public System {
public:
    Phase GetPhase() const override { return Phase::Update; }

    void OnUpdate(float dt) override {
        auto objects = World::Get().Query<TransformComponent, CharacterComponent,PhysicalAttributesComponent>();
		for (auto [entity, transform, character,ip] : objects.each()) {
            float cameraYaw = GetCameraYaw(entity);
			HandleMovement(transform, character, cameraYaw, dt);
			HandleJump(entity, transform, character, ip, dt);
			HandleRotation(transform, character, dt);
            UpdateState(character,ip);
        }
        auto& inputManager = Kiki::InputManager::get();
    }
    void OnStart() override {
        auto objects2 = World::Get().Query<MiscComponent>();
        glm::vec3 spawnPos = glm::vec3(0, 0, 0);
        //Entity playerEntity = NullEntity;
        for (auto [e,misc] : objects2.each()) {
        	if (misc.miscTag == MmiscTags::PLAYER) {
        		playerEntity = e;
        		World::Get().Registry().emplace<CharacterComponent>(e);
        	}
        	if (misc.miscTag == MmiscTags::SPAWN) {
        		auto* transform = World::Get().GetComponent<TransformComponent>(e);
        		if (transform) {
        			spawnPos = transform->position;
        		}
        	}
        }
        
        if (playerEntity != NullEntity) {
        	auto* character = World::Get().GetComponent<CharacterComponent>(playerEntity);
        	auto* transform = World::Get().GetComponent<TransformComponent>(playerEntity);
			auto* physics = World::Get().GetComponent<PhysicalAttributesComponent>(playerEntity);
        	if (character) {
        		character->spawnPosition = spawnPos;
        	}
        	if(transform){
        		transform->position = spawnPos;
                float modelRotationOffset = 180.0f;
                transform->rotation = glm::angleAxis(
                    glm::radians(character->facingYaw + modelRotationOffset),
                    glm::vec3(0, 1, 0)
                );
        		transform->dirty = true;
        	}
            if(physics){
                physics->isGroundedNeedsUpdate = true;
			}
        }
    }
    void OnStop() override {
       
    }
private:
    InputManager& inputManager = Kiki::InputManager::get();
	Entity playerEntity = NullEntity;
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
    void HandleMovement(
        TransformComponent& transform,
        CharacterComponent& character,
        float cameraYaw, float dt)
    {
        glm::vec2 inputDir = { 0, 0 };
        if (inputManager.isKeyDown(GLFW_KEY_W)) inputDir.y += 1.0f;
        if (inputManager.isKeyDown(GLFW_KEY_S)) inputDir.y -= 1.0f;
        if (inputManager.isKeyDown(GLFW_KEY_A)) inputDir.x -= 1.0f;
        if (inputManager.isKeyDown(GLFW_KEY_D)) inputDir.x += 1.0f;

        bool isRunning = inputManager.isKeyDown(GLFW_KEY_LEFT_SHIFT);
        float speed = isRunning ? character.runSpeed : character.walkSpeed;

        if (glm::length(inputDir) > 0.001f) {
			// normalize to prevent faster diagonal movement
            inputDir = glm::normalize(inputDir);

			// inputdir transform from character space to camera space
			// cameraYaw decides which direction is "forward" for the character
            float rad = glm::radians(cameraYaw);
            //glm::vec3 forward = { sin(rad), 0, cos(rad) };
            glm::vec3 forward = {-sin(rad), 0, -cos(rad) };
            glm::vec3 right = { cos(rad), 0, -sin(rad) };

            glm::vec3 moveDir = forward * inputDir.y + right * inputDir.x;

            character.velocity.x = moveDir.x * speed;
            character.velocity.z = moveDir.z * speed;

			// record target facing direction (character faces movement direction)
            //character.targetYaw = glm::degrees(atan2(moveDir.x, moveDir.z));
            character.targetYaw = glm::degrees(atan2(-moveDir.x, -moveDir.z));
        }
        else {
			// TODO: apply friction to slow down instead of stopping immediately
			// or just connect with physics system
            character.velocity.x = 0.0f;
            character.velocity.z = 0.0f;
            return;
        }
        transform.position += character.velocity * dt;
		//PhysicsService& physics = World::Get().Registry().ctx().get<PhysicsService>();
  //              physics.setEntityVelocity(playerEntity, character.velocity);
        transform.dirty = true;
    }
    void HandleJump(
		Entity entity,
        TransformComponent& transform,
        CharacterComponent& character,
        PhysicalAttributesComponent& ip,
        float dt)
    {
        if (inputManager.isKeyDown(GLFW_KEY_SPACE)
			&& character.state != CharacterState::Jumping
            ) {
			ip.impulse.y += character.jumpForce;
            character.state = CharacterState::Jumping;
        }
    }
	// smoothly rotate character to face movement direction
    void HandleRotation(TransformComponent& transform,
        CharacterComponent& character,
        float dt)
    {
		// keep current facing direction if not moving
        if (glm::length(glm::vec2(character.velocity.x,
            character.velocity.z)) < 0.001f)
            return;

		// interpolate facingYaw towards targetYaw
        float diff = character.targetYaw - character.facingYaw;

		// handle angle wrap-around (e.g. from 350 to 10 degrees should rotate 20 degrees, not -340)
        if (diff > 180.0f)  diff -= 360.0f;
        if (diff < -180.0f) diff += 360.0f;

        character.facingYaw += diff * character.rotateSpeed * dt;

        float modelRotationOffset = 180.0f;

		// update transform rotation to match facing direction
        transform.rotation = glm::angleAxis(
            glm::radians(character.facingYaw + modelRotationOffset),
            glm::vec3(0, 1, 0)
        );
        transform.dirty = true;
    }
    void UpdateState(CharacterComponent& character, PhysicalAttributesComponent& pa) {
        float currentSpeed = glm::length(glm::vec2(character.velocity.x, character.velocity.z));
        bool isMoving = glm::length(glm::vec2(character.velocity.x, character.velocity.z)) > 0.1f;

        if (character.state == CharacterState::Jumping) {
            if (character.velocity.y <= 0.0f && pa.isGrounded) { 
                character.state = isMoving ? CharacterState::Walking : CharacterState::Idle;
            }
            return;
        }

        if (isMoving) {
            if (currentSpeed > character.walkSpeed + 1.0f) {
                character.state = CharacterState::Running;
            }
            else {
                character.state = CharacterState::Walking;
            }
        }
        else {
            character.state = CharacterState::Idle;
        }
    }
};