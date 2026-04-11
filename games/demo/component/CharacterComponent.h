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
    float walkSpeed = 3.0f;
    float runSpeed = 6.0f;
    float jumpForce = 5000.0f;
	float rotateSpeed = 10.0f; // character rotation speed (for interpolating facing direction)

    CharacterState state = CharacterState::Idle;
    glm::vec3      velocity = { 0, 0, 0 };


	float          facingYaw = 0.0f;   // current facing direction
	float          targetYaw = 0.0f;   // target facing direction
};