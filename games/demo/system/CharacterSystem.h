#pragma once
#include <kiki.h>
#include "../component/CharacterComponent.h"
#include "../component/ThirdPersonCameraComponent.hpp"
class CharacterSystem : public System {
public:
    Phase GetPhase() const override { return Phase::Update; }

    void OnUpdate(float dt) override {
        auto objects = World::Get().Query<TransformComponent, CharacterComponent,ImpulseComponent>();
		for (auto [entity, transform, character,ip] : objects.each()) {
            float cameraYaw = GetCameraYaw(entity);
			HandleMovement(transform, character, cameraYaw, dt);
			HandleJump(entity, transform, character, ip, dt);
        }
        auto& inputManager = Kiki::InputManager::get();
    }
    void OnStart() override {
        
    }
    void OnStop() override {
       
    }
private:
    InputManager& inputManager = Kiki::InputManager::get();
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
            glm::vec3 forward = { sin(rad), 0, cos(rad) };
            glm::vec3 right = { cos(rad), 0, -sin(rad) };

            glm::vec3 moveDir = forward * inputDir.y + right * inputDir.x;

            character.velocity.x = moveDir.x * speed;
            character.velocity.z = moveDir.z * speed;

			// record target facing direction (character faces movement direction)
            character.targetYaw = glm::degrees(atan2(moveDir.x, moveDir.z));
        }
        else {
			// TODO: apply friction to slow down instead of stopping immediately
			// or just connect with physics system
            character.velocity.x = 0.0f;
            character.velocity.z = 0.0f;
        }

        transform.position += character.velocity * dt;
        transform.dirty = true;
    }
    void HandleJump(
		Entity entity,
        TransformComponent& transform,
        CharacterComponent& character,
		ImpulseComponent& ip,
        float dt)
    {
        if (inputManager.isKeyDown(GLFW_KEY_SPACE)
            //&& PhysicsSystem::isGrounded(entity,0.2f)
            ) {
			ip.impulse.y += character.jumpForce;
            character.state = CharacterState::Jumping;
        }
    }
};