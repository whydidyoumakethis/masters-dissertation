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

inline std::string to_string(CharacterState state) {
    switch (state) {
    case CharacterState::Idle:    return "Idle";
    case CharacterState::Walking: return "Walking";
    case CharacterState::Running: return "Running";
    case CharacterState::Jumping: return "Jumping";
    default:                      return "Unknown";
    }
}

struct CharacterComponent {
    float walkSpeed = 5.0f;
    float runSpeed = 18.0f;
    float jumpForce = 15.0f;
	float rotateSpeed = 10.0f; // character rotation speed (for interpolating facing direction)

    CharacterState state = CharacterState::Idle;
    glm::vec3      velocity = { 0, 0, 0 };
	glm::vec3      spawnPosition = { 0, 0, 0 }; // for respawning after falling off the level

	float          facingYaw = 0.0f;   // current facing direction
	float          targetYaw = 0.0f;   // target facing direction
};