#pragma once
#include <kiki.h>
#include "../component/CharacterComponent.h"
#include "../component/ThirdPersonCameraComponent.hpp"
#include "Animation/AnimationComponent.h"
#include"../Events.h"
class CharacterSystem : public System {
public:
    Phase GetPhase() const override { return Phase::Update; }

    void OnUpdate(float dt) override {
        auto objects = World::Get().Query<TransformComponent, CharacterComponent,PhysicalAttributesComponent>();
		for (auto [entity, transform, character,ip] : objects.each()) {
            if (character.jumpTimer > 0.0f) {
                character.jumpTimer -= dt;
            }

            float cameraYaw = GetCameraYaw(entity);
			HandleMovement(transform, character, ip, cameraYaw, dt);
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
        MessageCenter::Subscribe<TimerTriggerEvent, &CharacterSystem::OnTimerTrigger>(this);

    }
    void OnStop() override {
       
    }
private:
    InputManager& inputManager = Kiki::InputManager::get();
	Entity playerEntity = NullEntity;
	bool isDoubleJumping = false;
	float dashTimer = 0.0f;// for dash ability cooldown
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
        PhysicalAttributesComponent& ip,
        float cameraYaw, float dt)
    {
        glm::vec2 inputDir = { 0, 0 };
        if (inputManager.isKeyDown(GLFW_KEY_W)) inputDir.y += 1.0f;
        if (inputManager.isKeyDown(GLFW_KEY_S)) inputDir.y -= 1.0f;
        if (inputManager.isKeyDown(GLFW_KEY_A)) inputDir.x -= 1.0f;
        if (inputManager.isKeyDown(GLFW_KEY_D)) inputDir.x += 1.0f;

        if (ip.isGrounded) {
            character.currentMaxSpeed = inputManager.isKeyDown(GLFW_KEY_LEFT_SHIFT) ? character.runSpeed : character.walkSpeed;
        }
        //bool isRunning = inputManager.isKeyDown(GLFW_KEY_LEFT_SHIFT) && ip.isGrounded;
        float speed = character.currentMaxSpeed;
        if (character.hasAbility(Ability::SpeedBoost)) {
            speed *= 2;
        }

        PhysicsService& physics = World::Get().Registry().ctx().get<PhysicsService>();
        auto* rb = World::Get().GetComponent<RigidBodyComponent>(playerEntity);
        JPH::Vec3 currentJoltVel = physics._manager.GetBodyInterface().GetLinearVelocity(rb->bodyID);

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

            glm::vec3 newVel = glm::vec3(character.velocity.x, currentJoltVel.GetY(), character.velocity.z);
            physics.setEntityVelocity(playerEntity, newVel);
        }
        else {
			// TODO: apply friction to slow down instead of stopping immediately
			// or just connect with physics system
            if (ip.isGrounded) {
                character.velocity.x = 0.0f;
                character.velocity.z = 0.0f;
                glm::vec3 newVel = glm::vec3(0.0f, currentJoltVel.GetY(), 0.0f);
                physics.setEntityVelocity(playerEntity, newVel);
            }
            else {
                character.velocity.x = currentJoltVel.GetX();
                character.velocity.z = currentJoltVel.GetZ();
            }
        }
        //if (character.hasAbility(Ability::SpeedBoost)) {
        //    transform.position += character.velocity * dt * 1.5f; // Apply speed boost multiplier
        //}
        //else transform.position += character.velocity * dt;
		//PhysicsService& physics = World::Get().Registry().ctx().get<PhysicsService>();
  //              physics.setEntityVelocity(playerEntity, character.velocity);

        //transform.position += character.velocity * dt;
        transform.dirty = true;
    }
    void HandleJump(
        Entity entity,
        TransformComponent& transform,
        CharacterComponent& character,
        PhysicalAttributesComponent& ip,
        float dt)
    {
        if (inputManager.isKeyJustDown(GLFW_KEY_SPACE) ){
            if (character.state != Kiki::CharacterState::Jumping) {
                PhysicsService& physics = World::Get().Registry().ctx().get<PhysicsService>();
                auto* rb = World::Get().GetComponent<RigidBodyComponent>(entity);
                JPH::Vec3 currentVel = physics._manager.GetBodyInterface().GetLinearVelocity(rb->bodyID);

                //AUDIO TEST!!!
                //Kiki::AudioSystem::PlayOneShot("sounds/cao.mp3");

                physics._manager.GetBodyInterface().SetLinearVelocity(rb->bodyID, JPH::Vec3(currentVel.GetX(), character.jumpForce, currentVel.GetZ()));

                character.state = Kiki::CharacterState::Jumping;
                character.jumpTimer = 0.15f;
            }
            else if (character.hasAbility(Ability::DoubleJump) && !isDoubleJumping) {
                PhysicsService& physics = World::Get().Registry().ctx().get<PhysicsService>();
                auto* rb = World::Get().GetComponent<RigidBodyComponent>(entity);
                JPH::Vec3 currentVel = physics._manager.GetBodyInterface().GetLinearVelocity(rb->bodyID);
                physics._manager.GetBodyInterface().SetLinearVelocity(rb->bodyID, JPH::Vec3(currentVel.GetX(), character.jumpForce, currentVel.GetZ()));
                character.state = Kiki::CharacterState::Jumping;
                character.jumpTimer = 0.15f;
                isDoubleJumping = true; // prevent further double jumps until grounded again
			}
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
        bool isMoving = glm::length(glm::vec2(character.velocity.x, character.velocity.z)) > 0.1f;
        float currentSpeed = glm::length(glm::vec2(character.velocity.x, character.velocity.z));

        PhysicsService& physics = World::Get().Registry().ctx().get<PhysicsService>();
        auto* rb = World::Get().GetComponent<RigidBodyComponent>(playerEntity);
        float realVy = physics._manager.GetBodyInterface().GetLinearVelocity(rb->bodyID).GetY();

        if (character.state == Kiki::CharacterState::Jumping) {
            if (character.jumpTimer <= 0.0f && pa.isGrounded) {
                character.state = isMoving ? Kiki::CharacterState::Walking : Kiki::CharacterState::Idle;
                isDoubleJumping = false; // reset double jump when grounded
            }
        }
        else {
            if (isMoving) {
                if (currentSpeed > character.walkSpeed + 1.0f) {
                    character.state = Kiki::CharacterState::Running;
                }
                else {
                    character.state = Kiki::CharacterState::Walking;
                }
            }
            else {
                character.state = Kiki::CharacterState::Idle;
            }
        }

        Kiki::AnimationComponent* animComp = World::Get().GetComponent<Kiki::AnimationComponent>(playerEntity);
        if (animComp) {
            bool shouldLoop = (character.state != Kiki::CharacterState::Jumping);

            if (animComp->currentState != character.state) {
                animComp->ChangeState(character.state, shouldLoop);
            }
        }
    }
     void OnTimerTrigger(const TimerTriggerEvent& e) {
        auto* cc = World::Get().GetComponent<CharacterComponent>(playerEntity);
        std::vector<float>& timelimits = cc->timeLimits;
        float elapsed = e.elapsedTime;
        for (size_t i = 0; i < timelimits.size(); ++i) {
			if (cc->isDone[i]) continue; // skip already completed tiers
            if (timelimits[i] > elapsed) {
                cc->isDone[i] = true;
                spdlog::info("You completed the level in {:.2f} seconds! You earned a new ability!", elapsed);
                if (i == 0) {
                    cc->grantAbility(Ability::DoubleJump);
					spdlog::info("You earned the double jump ability!");
                }
                else if (i == 1) {
                    cc->grantAbility(Ability::SpeedBoost);
					spdlog::info("You earned the speed boost ability!");
                }
                else if (i == 2) {
                    cc->grantAbility(Ability::Dash);
					spdlog::info("You earned the dash ability!");
                }
                else if (i == 3) {
					// completed level logic here
					spdlog::info("Congratulations! You completed the level! ");
                }
                break;
            }
        }
    }
};