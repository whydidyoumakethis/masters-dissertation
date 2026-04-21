#pragma once
#include <glm/glm.hpp>
enum class CharacterState {
    Idle,
    Walking,
    Running,
    Jumping,
//    Falling,
//    Landing,
};
struct CharacterComponent {
    float walkSpeed = 5.0f;
    float runSpeed = 18.0f;
    float jumpForce = 10000.0f;
	float rotateSpeed = 10.0f; // character rotation speed (for interpolating facing direction)

    CharacterState state = CharacterState::Idle;
    glm::vec3      velocity = { 0, 0, 0 };
	glm::vec3      spawnPosition = { 0, 0, 0 }; // for respawning after falling off the level

	float          facingYaw = 0.0f;   // current facing direction
	float          targetYaw = 0.0f;   // target facing direction
};